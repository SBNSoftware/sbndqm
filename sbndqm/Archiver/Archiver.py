#!/usr/bin/env python3
"""
SBND DQM Archiver
=================

Reads online-monitoring metrics from Redis streams and archives them into
the PostgreSQL run-conditions database.

Stream/table layout
-------------------
The PostgreSQL table <metric_config> (e.g. runcon_prd.monitor_map) defines one
row per (group_name, metric, channel_id):

    group_name | metric | channel_id | monitor_type | postgres_table | time_interval | ...

For each row the corresponding Redis stream is named

    <group_name>:<channel_id>:<metric>:archiving

and the destination table is <schema>.<postgres_table> with columns
(CHANNEL_ID, SMPL_TIME, SMPL_VALUE).

Deduplication / state management
--------------------------------
For every stream the archiver keeps a high-water mark in Redis under the key

    <stream>_LatestCompleted

holding the ID of the last stream entry that has been archived. Each pass
only reads entries *strictly newer* than the high-water mark, so data is
never processed twice. The mark is advanced only *after* the corresponding
rows have been committed to PostgreSQL; if the archiver dies between commit
and mark update, the re-inserted rows are silently skipped via
INSERT ... ON CONFLICT DO NOTHING (the tables have a (channel_id, smpl_time)
primary key), so the pipeline is safe on both sides.

Aggregation
-----------
Entries are grouped into epoch-aligned time windows of `time_interval`
seconds (from the monitor map) and reduced with the configured monitor type:

    0 = MEAN, 1 = MEDIAN, 2 = MODE, 3 = MAX, 4 = MIN, 5 = LAST

One row is written per (stream, window), stamped with the time of the last
entry in the window. A time interval of 0 (or --interval-override 0)
archives every sample individually.

Modes
-----
* one-shot (default): archive everything new, then exit. Suitable for cron
  and for the offline/ROOT-file workflow.
* --daemon: loop forever, scanning every --poll-interval seconds and
  emitting an `archiver_heartbeat` stream entry each pass.

After every pass the archiver publishes the timestamp of the most recent
metric entry it has seen to Redis (key `archiver_last_entry_timestamp_ms`
and stream `archiver_last_timestamp`) for display on the monitoring website.

The trailing (possibly still-filling) time window is normally held back so
it can be completed later; pass --flush (offline workflow) to archive it
immediately.
"""

import argparse
import json
import logging
import os
import signal
import struct
import sys
import time
from collections import Counter, defaultdict
from multiprocessing import Process
from statistics import median

import psycopg2
import psycopg2.extras
import redis

DEFAULT_CONFIG = '/home/nfs/sbnd/DQM_SPACK/dqm_env_2/sbndqm/sbndqm/Archiver/ArchiverConfig.json'
STATE_SUFFIX = '_LatestCompleted'
LAST_ENTRY_TS_KEY = 'archiver_last_entry_timestamp_ms'
LAST_ENTRY_TS_STREAM = 'archiver_last_timestamp'

log = logging.getLogger('archiver')

# ---------------------------------------------------------------------------
# Configuration / connections
# ---------------------------------------------------------------------------

def load_config(path):
    with open(path) as f:
        return json.load(f)


def setup_logging(config, quiet=False):
    handlers = []
    logcfg = config.get('logging', {})
    if logcfg.get('directory') and logcfg.get('name'):
        mode = 'w' if logcfg.get('overwrite') else 'a'
        path = os.path.join(logcfg['directory'], logcfg['name'])
        try:
            handlers.append(logging.FileHandler(path, mode=mode))
        except OSError as err:
            print('WARNING: cannot open log file %s (%s); logging to stderr only' % (path, err))
    if not quiet:
        handlers.append(logging.StreamHandler())
    logging.basicConfig(level=logging.INFO,
                        format='%(asctime)s %(levelname)s %(message)s',
                        datefmt='%Y-%m-%d %H:%M:%S',
                        handlers=handlers)


def connect_redis(config, password=None):
    rcfg = config['redis']
    r = redis.Redis(host=rcfg['hostname'], port=rcfg['port'],
                    password=password or rcfg.get('password'),
                    socket_timeout=rcfg.get('timeout', 30))
    r.ping()
    log.info('Connected to Redis at %s:%s', rcfg['hostname'], rcfg['port'])
    return r


def connect_postgres(config):
    pcfg = config['postgresql']
    p = psycopg2.connect(host=pcfg['hostname'], database=pcfg['database'],
                         user=pcfg['user'], port=pcfg['port'],
                         password=pcfg.get('password'))
    log.info('Connected to PostgreSQL at %s:%s/%s', pcfg['hostname'],
             pcfg['port'], pcfg['database'])
    return p


# ---------------------------------------------------------------------------
# Monitor map -> stream specifications
# ---------------------------------------------------------------------------

class StreamSpec:
    __slots__ = ('stream', 'group', 'metric', 'channel', 'monitor_type',
                 'table', 'interval_ms')

    def __init__(self, group, metric, channel, monitor_type, table, interval_s):
        self.group = group
        self.metric = metric
        self.channel = channel
        self.monitor_type = monitor_type
        self.table = table
        self.interval_ms = int(float(interval_s) * 1000)
        self.stream = '%s:%s:%s:archiving' % (group, channel, metric)


def load_stream_specs(pgconn, config, args):
    """Build the list of streams to archive from the monitor-map table."""
    metric_config = config['postgresql']['metric_config']
    schema = metric_config.split('.')[0] if '.' in metric_config else 'public'
    cur = pgconn.cursor()
    cur.execute('SELECT table_name FROM information_schema.tables '
                'WHERE table_schema = %s', (schema,))
    existing_tables = {row[0] for row in cur.fetchall()}
    cur.execute(
        'SELECT group_name, metric, channel_id, monitor_type, postgres_table,'
        '       time_interval FROM %s' % metric_config)
    specs, missing = [], Counter()
    for group, metric, channel, mtype, table, interval in cur.fetchall():
        if args.group and group not in args.group:
            continue
        if args.metric and metric not in args.metric:
            continue
        if args.channel is not None and channel != args.channel:
            continue
        if table not in existing_tables:
            missing[table] += 1
            continue
        if args.interval_override is not None:
            interval = args.interval_override
        specs.append(StreamSpec(group, metric, channel, mtype, table, interval))
    cur.close()
    for table, n in missing.items():
        log.warning('Monitor map references missing table %s.%s; '
                    'skipping its %d streams', schema, table, n)
    log.info('Monitor map: %d streams selected for archiving', len(specs))
    return specs


# ---------------------------------------------------------------------------
# Stream entry decoding
# ---------------------------------------------------------------------------

_STRUCT_FORMAT = {
    'int8_t': 'b', 'int16_t': 'h', 'int32_t': 'i', 'int64_t': 'q',
    'uint8_t': 'B', 'uint16_t': 'H', 'uint32_t': 'I', 'uint64_t': 'Q',
    'float': 'f', 'double': 'd',
}


def decode_value(fields):
    """Decode the value of a metric stream entry.

    Entries are either {'val': b'1.23'} / {'dat': ...} with a stringified
    number, or {<ctype>: <packed binary>} as produced by the DAQ metric
    plugins (e.g. {'float': b'\\x00\\x00\\xe0>'}).
    """
    for key, val in fields.items():
        name = key.decode() if isinstance(key, bytes) else key
        if name in ('val', 'dat'):
            return float(val)
        fmt = _STRUCT_FORMAT.get(name)
        if fmt is not None:
            return float(struct.unpack('<' + fmt, val[:struct.calcsize(fmt)])[0])
    raise ValueError('unrecognized stream entry fields: %r' % fields)


def entry_ms(entry_id):
    """Millisecond part of a Redis stream ID b'1781201854574-0'."""
    if isinstance(entry_id, bytes):
        entry_id = entry_id.decode()
    return int(entry_id.split('-')[0])


def next_id(entry_id):
    """Smallest stream ID strictly greater than entry_id (for inclusive XRANGE)."""
    if isinstance(entry_id, bytes):
        entry_id = entry_id.decode()
    ms, _, seq = entry_id.partition('-')
    return '%s-%d' % (ms, int(seq or 0) + 1)


# ---------------------------------------------------------------------------
# Aggregation (monitor types)
# ---------------------------------------------------------------------------

def aggregate(values, monitor_type):
    if monitor_type == 0:
        return sum(values) / len(values)
    if monitor_type == 1:
        return median(values)
    if monitor_type == 2:
        return Counter(values).most_common(1)[0][0]
    if monitor_type == 3:
        return max(values)
    if monitor_type == 4:
        return min(values)
    if monitor_type == 5:
        return values[-1]
    raise ValueError('unknown monitor type %d' % monitor_type)


def build_rows(spec, entries, flush, now_ms, grace_ms):
    """Aggregate stream entries into archive rows.

    Returns (rows, hwm) where rows is a list of (channel, ts_ms, value) and
    hwm is the ID of the last entry folded into those rows (None if nothing
    is ready to be archived yet).
    """
    # Group entries into epoch-aligned windows. interval <= 0 archives
    # every sample individually.
    windows = []  # list of [window_idx, [(id, ms, value), ...]]
    for eid, fields in entries:
        try:
            value = decode_value(fields)
        except (ValueError, struct.error) as err:
            log.warning('%s: skipping undecodable entry %s (%s)', spec.stream, eid, err)
            continue
        ms = entry_ms(eid)
        widx = ms // spec.interval_ms if spec.interval_ms > 0 else ms
        if windows and windows[-1][0] == widx:
            windows[-1][1].append((eid, ms, value))
        else:
            windows.append([widx, [(eid, ms, value)]])

    rows, hwm = [], None
    for widx, items in windows:
        if spec.interval_ms > 0 and not flush:
            window_end = (widx + 1) * spec.interval_ms
            if window_end > now_ms - grace_ms:
                break  # window may still receive data; hold it for later
        values = [v for _, _, v in items]
        last_id, last_ms, _ = items[-1]
        rows.append((spec.channel, last_ms / 1000.0,
                     aggregate(values, spec.monitor_type)))
        hwm = last_id
    return rows, hwm


# ---------------------------------------------------------------------------
# Archiving passes
# ---------------------------------------------------------------------------

def archive_pass(r, p, specs, schema, args):
    """One scan over all streams: read new entries, aggregate, write, advance
    the high-water marks.

    Returns (rows_written, latest_entry_ms) where latest_entry_ms is the
    millisecond timestamp of the newest metric entry seen in this pass
    (from new stream data, or from an existing high-water mark when a stream
    has no unread entries).
    """
    now_ms = int(time.time() * 1000)
    grace_ms = int(args.grace * 1000)
    total_rows = 0
    latest_entry_ms = 0

    for start in range(0, len(specs), args.chunk_size):
        chunk = specs[start:start + args.chunk_size]

        # 1. Fetch high-water marks for the chunk.
        pipe = r.pipeline(transaction=False)
        for spec in chunk:
            pipe.get(spec.stream + STATE_SUFFIX)
        marks = pipe.execute()

        # 2. Read everything newer than each mark.
        pipe = r.pipeline(transaction=False)
        for spec, mark in zip(chunk, marks):
            lo = next_id(mark) if mark and mark != b'0' else '-'
            pipe.xrange(spec.stream, min=lo, max='+')
        results = pipe.execute()

        # 3. Aggregate into rows, per destination table.
        table_rows = defaultdict(list)
        table_marks = defaultdict(dict)
        for spec, mark, entries in zip(chunk, marks, results):
            if entries:
                latest_entry_ms = max(latest_entry_ms,
                                      max(entry_ms(eid) for eid, _ in entries))
            elif mark and mark != b'0':
                latest_entry_ms = max(latest_entry_ms, entry_ms(mark))
            if not entries:
                continue
            rows, hwm = build_rows(spec, entries, args.flush, now_ms, grace_ms)
            if rows:
                table_rows[spec.table].extend(rows)
                table_marks[spec.table][spec.stream + STATE_SUFFIX] = hwm

        if not table_rows:
            continue

        # 4. Commit to PostgreSQL first ... A failure on one table (e.g. a
        # stale monitor-map entry) must not poison the other tables, so each
        # insert runs inside a savepoint; failed tables keep their old
        # high-water marks and are retried on the next pass.
        new_marks = {}
        cur = p.cursor()
        for table, rows in table_rows.items():
            cur.execute('SAVEPOINT archive_table')
            try:
                psycopg2.extras.execute_values(
                    cur,
                    'INSERT INTO {}.{} (channel_id, smpl_time, smpl_value) '
                    'VALUES %s ON CONFLICT DO NOTHING'.format(schema, table),
                    rows,
                    template='(%s, to_timestamp(%s), %s)')
            except psycopg2.Error as err:
                cur.execute('ROLLBACK TO SAVEPOINT archive_table')
                log.error('insert into %s.%s failed (%d rows): %s',
                          schema, table, len(rows), str(err).strip())
            else:
                total_rows += len(rows)
                new_marks.update(table_marks[table])
            cur.execute('RELEASE SAVEPOINT archive_table')
        p.commit()
        cur.close()

        # 5. ... then advance the high-water marks.
        if new_marks:
            pipe = r.pipeline(transaction=False)
            for key, hwm in new_marks.items():
                pipe.set(key, hwm)
            pipe.execute()

    return total_rows, latest_entry_ms


_UPDATE_LAST_TS = """
local cur = tonumber(redis.call('GET', KEYS[1]) or '0')
local ts = tonumber(ARGV[1])
if ts > cur then
  redis.call('SET', KEYS[1], ARGV[1])
  return 1
end
return 0
"""


def publish_last_timestamp(r, ts_ms):
    """Publish the newest metric-entry timestamp for the monitoring website.

    Two Redis objects are updated so either stream- or key-based consumers
    can read the value:

      * key   archiver_last_entry_timestamp_ms  -> milliseconds since epoch (string)
      * stream archiver_last_timestamp         -> {'double': <seconds since epoch>}
    """
    if ts_ms <= 0:
        return
    try:
        r.eval(_UPDATE_LAST_TS, 1, LAST_ENTRY_TS_KEY, str(ts_ms))
        r.xadd(LAST_ENTRY_TS_STREAM,
               {'double': struct.pack('<d', ts_ms / 1000.0)},
               maxlen=1)
    except redis.RedisError as err:
        log.warning('failed to publish last entry timestamp: %s', err)


def heartbeat(r):
    try:
        r.xadd('archiver_heartbeat', {'float': struct.pack('<f', 0.0)}, maxlen=1)
    except redis.RedisError as err:
        log.warning('heartbeat failed: %s', err)


def run_worker(worker_id, specs, config, args):
    """Entry point of one archiving process (handles its share of streams)."""
    r = connect_redis(config, args.password)
    p = connect_postgres(config)
    schema = config['postgresql']['metric_config'].split('.')[0] \
        if '.' in config['postgresql']['metric_config'] else 'public'

    while True:
        t0 = time.time()
        latest_ms = 0
        try:
            n, latest_ms = archive_pass(r, p, specs, schema, args)
        except (redis.RedisError, psycopg2.Error) as err:
            log.error('worker %d: pass failed: %s', worker_id, err)
            p.rollback()
            n = -1
        dt = time.time() - t0
        if n >= 0:
            log.info('worker %d: archived %d rows from %d streams in %.1fs',
                     worker_id, n, len(specs), dt)
            publish_last_timestamp(r, latest_ms)
        if not args.daemon:
            break
        if worker_id == 0:
            heartbeat(r)
        time.sleep(max(0.0, args.poll_interval - dt))


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    ap = argparse.ArgumentParser(
        description='Archive SBND DQM metrics from Redis into PostgreSQL.')
    ap.add_argument('-c', '--config', default=DEFAULT_CONFIG,
                    help='path to ArchiverConfig.json (default: %(default)s)')
    ap.add_argument('-pw', '--password', default=None,
                    help='Redis password (overrides the config file)')
    ap.add_argument('-pr', '--processes', type=int, default=1,
                    help='number of worker processes (default: 1)')
    ap.add_argument('-gn', '--group', action='append', default=None,
                    metavar='GROUP', help='only archive this metric group '
                    '(repeatable, e.g. -gn tpc_channel -gn PMT)')
    ap.add_argument('-mn', '--metric', action='append', default=None,
                    metavar='METRIC', help='only archive this metric name (repeatable)')
    ap.add_argument('-ch', '--channel', type=int, default=None,
                    help='only archive this channel id (for debugging)')
    ap.add_argument('--daemon', action='store_true',
                    help='run forever, polling for new data (default: one shot)')
    ap.add_argument('--poll-interval', type=float, default=60.0,
                    help='seconds between scans in daemon mode (default: 60)')
    ap.add_argument('--flush', action='store_true',
                    help='also archive the trailing, possibly incomplete time '
                         'window (use for offline/one-off workflows)')
    ap.add_argument('--grace', type=float, default=60.0,
                    help='seconds to wait after a window closes before '
                         'archiving it (default: 60)')
    ap.add_argument('--interval-override', type=float, default=None,
                    metavar='SECONDS', help='override monitor-map time_interval; '
                    '0 archives every sample without aggregation')
    ap.add_argument('--chunk-size', type=int, default=500,
                    help='streams per Redis pipeline batch (default: 500)')
    ap.add_argument('--quiet', action='store_true', help='no console logging')
    args = ap.parse_args()

    config = load_config(args.config)
    setup_logging(config, args.quiet)
    signal.signal(signal.SIGINT, lambda *_: sys.exit(0))
    signal.signal(signal.SIGTERM, lambda *_: sys.exit(0))

    log.info('Starting Archiver (mode=%s, flush=%s, processes=%d)',
             'daemon' if args.daemon else 'one-shot', args.flush, args.processes)

    p = connect_postgres(config)
    specs = load_stream_specs(p, config, args)
    p.close()
    if not specs:
        log.warning('No streams matched the selection; nothing to do.')
        return

    if args.processes <= 1:
        run_worker(0, specs, config, args)
    else:
        workers = []
        for i in range(args.processes):
            share = specs[i::args.processes]
            proc = Process(target=run_worker, args=(i, share, config, args))
            proc.start()
            workers.append(proc)
        for proc in workers:
            proc.join()

    log.info('Archiver finished.')


if __name__ == '__main__':
    main()
