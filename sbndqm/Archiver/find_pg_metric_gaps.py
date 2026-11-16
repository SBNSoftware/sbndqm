#!/usr/bin/env python3
"""
Find periods with no archived DQM metrics in PostgreSQL.

Scans every metric table listed in monitor_map (that exists in the schema)
and reports time windows where no table has any smpl_time entry for longer
than a configurable threshold (default: 3 days).

For each calendar day that has at least one metric anywhere in the archive,
the script tracks the earliest and latest smpl_time seen that day across all
tables. A gap is reported when the interval between the last metric before
the hole and the first metric after it exceeds the threshold.
"""

import argparse
import json
import sys
import time
from collections import defaultdict
from datetime import timedelta

import psycopg2

DEFAULT_CONFIG = '/home/nfs/sbnddqm/ArchiverConfig.json'


def load_config(path):
    with open(path) as f:
        return json.load(f)


def connect_postgres(config):
    pcfg = config['postgresql']
    return psycopg2.connect(
        host=pcfg['hostname'],
        port=pcfg['port'],
        dbname=pcfg['database'],
        user=pcfg['user'],
        password=pcfg.get('password'),
    )


def metric_schema(config):
    metric_config = config['postgresql']['metric_config']
    if '.' in metric_config:
        return metric_config.split('.', 1)[0]
    return 'public'


def list_metric_tables(conn, config):
    """Return postgres_table names from monitor_map that exist in PG."""
    schema = metric_schema(config)
    metric_config = config['postgresql']['metric_config']
    cur = conn.cursor()
    cur.execute(
        """
        SELECT t.table_name
        FROM information_schema.tables t
        JOIN (
            SELECT DISTINCT postgres_table AS table_name
            FROM %s
        ) m ON m.table_name = t.table_name
        WHERE t.table_schema = %%s
        ORDER BY 1
        """ % metric_config,
        (schema,),
    )
    return [row[0] for row in cur.fetchall()]


def collect_daily_bounds(conn, schema, tables, since=None, until=None, verbose=False):
    """Map each active day -> [earliest smpl_time, latest smpl_time] globally."""
    day_bounds = defaultdict(lambda: [None, None])
    where = []
    params = []
    if since is not None:
        where.append('smpl_time >= %s')
        params.append(since)
    if until is not None:
        where.append('smpl_time < %s')
        params.append(until)
    where_sql = (' WHERE ' + ' AND '.join(where)) if where else ''

    cur = conn.cursor()
    for i, table in enumerate(tables, 1):
        t0 = time.time()
        cur.execute(
            """
            SELECT date_trunc('day', smpl_time) AS d,
                   MIN(smpl_time),
                   MAX(smpl_time)
            FROM %s.%s
            %s
            GROUP BY 1
            """ % (schema, table, where_sql),
            params,
        )
        rows = cur.fetchall()
        for day, first_ts, last_ts in rows:
            bounds = day_bounds[day]
            if bounds[0] is None or first_ts < bounds[0]:
                bounds[0] = first_ts
            if bounds[1] is None or last_ts > bounds[1]:
                bounds[1] = last_ts
        if verbose:
            print(
                '[{}/{}] {} ({} active days, {:.1f}s)'.format(
                    i, len(tables), table, len(rows), time.time() - t0,
                ),
                file=sys.stderr,
            )
    return day_bounds


def find_gaps(day_bounds, gap_threshold):
    gaps = []
    days = sorted(day_bounds)
    for prev_day, next_day in zip(days, days[1:]):
        gap_start = day_bounds[prev_day][1]
        gap_end = day_bounds[next_day][0]
        duration = gap_end - gap_start
        if duration > gap_threshold:
            gaps.append((gap_start, gap_end, duration))
    return gaps, days


def format_duration(delta):
    total_seconds = int(delta.total_seconds())
    days, rem = divmod(total_seconds, 86400)
    hours, rem = divmod(rem, 3600)
    minutes, seconds = divmod(rem, 60)
    parts = []
    if days:
        parts.append('{} day{}'.format(days, 's' if days != 1 else ''))
    if hours:
        parts.append('{} hour{}'.format(hours, 's' if hours != 1 else ''))
    if minutes and not days:
        parts.append('{} minute{}'.format(minutes, 's' if minutes != 1 else ''))
    if seconds and not days and not hours:
        parts.append('{} second{}'.format(seconds, 's' if seconds != 1 else ''))
    return ', '.join(parts) if parts else '0 seconds'


def format_ts(ts):
    return ts.strftime('%Y-%m-%d %H:%M:%S %Z').strip()


def main():
    parser = argparse.ArgumentParser(
        description='Report PostgreSQL metric archive gaps (periods with no data).',
    )
    parser.add_argument(
        '-c', '--config', default=DEFAULT_CONFIG,
        help='Archiver JSON config (default: %(default)s)',
    )
    parser.add_argument(
        '--gap-days', type=float, default=3.0,
        help='Minimum gap length in days (default: %(default)s)',
    )
    parser.add_argument(
        '--since', metavar='TIMESTAMP',
        help='Only consider metrics at or after this time (ISO-8601 or YYYY-MM-DD)',
    )
    parser.add_argument(
        '--until', metavar='TIMESTAMP',
        help='Only consider metrics before this time (ISO-8601 or YYYY-MM-DD)',
    )
    parser.add_argument(
        '-v', '--verbose', action='store_true',
        help='Print per-table scan progress on stderr',
    )
    parser.add_argument(
        '--json', action='store_true',
        help='Emit gaps as JSON on stdout',
    )
    args = parser.parse_args()

    gap_threshold = timedelta(days=args.gap_days)
    config = load_config(args.config)
    schema = metric_schema(config)
    pcfg = config['postgresql']

    conn = connect_postgres(config)
    try:
        tables = list_metric_tables(conn, config)
        if not tables:
            print('No metric tables found in {}.monitor_map'.format(schema), file=sys.stderr)
            return 1

        if args.verbose:
            print(
                'Scanning {} tables in {} @ {}:{}'.format(
                    len(tables), pcfg['database'], pcfg['hostname'], pcfg['port'],
                ),
                file=sys.stderr,
            )

        t0 = time.time()
        day_bounds = collect_daily_bounds(
            conn, schema, tables,
            since=args.since, until=args.until, verbose=args.verbose,
        )
        elapsed = time.time() - t0

        if not day_bounds:
            print('No metric timestamps found in the selected range.', file=sys.stderr)
            return 0

        gaps, days = find_gaps(day_bounds, gap_threshold)
        coverage_start = day_bounds[days[0]][0]
        coverage_end = day_bounds[days[-1]][1]

        if args.json:
            payload = {
                'database': pcfg['database'],
                'host': pcfg['hostname'],
                'port': pcfg['port'],
                'schema': schema,
                'tables_scanned': len(tables),
                'gap_threshold_days': args.gap_days,
                'coverage_start': coverage_start.isoformat(),
                'coverage_end': coverage_end.isoformat(),
                'active_days': len(days),
                'scan_seconds': round(elapsed, 2),
                'gaps': [
                    {
                        'last_metric_before': gap_start.isoformat(),
                        'first_metric_after': gap_end.isoformat(),
                        'duration_seconds': gap.total_seconds(),
                        'duration_human': format_duration(gap),
                    }
                    for gap_start, gap_end, gap in gaps
                ],
            }
            json.dump(payload, sys.stdout, indent=2)
            sys.stdout.write('\n')
            return 0

        print('PostgreSQL metric coverage gaps (> {:.1f} days without data)'.format(args.gap_days))
        print('Database: {} @ {}:{}'.format(pcfg['database'], pcfg['hostname'], pcfg['port']))
        print('Schema: {} ({} tables scanned in {:.1f}s)'.format(schema, len(tables), elapsed))
        print('Coverage: {} -> {} ({} days with data)'.format(
            format_ts(coverage_start), format_ts(coverage_end), len(days),
        ))
        print('Gaps found: {}'.format(len(gaps)))
        print()

        if not gaps:
            print('No gaps longer than {:.1f} days.'.format(args.gap_days))
            return 0

        for i, (gap_start, gap_end, duration) in enumerate(gaps, 1):
            print('Gap {}:'.format(i))
            print('  last metric before: {}'.format(format_ts(gap_start)))
            print('  first metric after: {}'.format(format_ts(gap_end)))
            print('  duration:           {} ({})'.format(
                format_duration(duration), duration,
            ))
            print()

        return 0
    finally:
        conn.close()


if __name__ == '__main__':
    sys.exit(main())
