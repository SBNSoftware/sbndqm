#!/usr/bin/env python3
"""
Trim older Redis stream data to reclaim memory.

Metric streams (those ending in :archiving, :fast, :slow, or :nearline) are
only trimmed *up to* the archiver high-water mark (<stream>_LatestCompleted).
Entries at or before that mark have already been written to PostgreSQL; anything
newer is always kept, even when applying the keep-fraction.

Internal archiver status streams (archiver_cpu*, archiver_mem, etc.) are not
tracked in PostgreSQL and are trimmed by entry count as before.

Streams that look like metrics but have no high-water mark are skipped so
unarchived data is never deleted.

Usage:
    python3 flush_redis_old_half.py --dry-run
    python3 flush_redis_old_half.py
    python3 flush_redis_old_half.py --keep-fraction 0.25
"""

import argparse
import inspect
import json
import logging
import time

import redis

DEFAULT_CONFIG = '/home/nfs/sbnddqm/ArchiverConfig.json'
STATE_SUFFIX = '_LatestCompleted'
METRIC_SUFFIXES = (':archiving', ':fast', ':slow', ':nearline')

log = logging.getLogger('flush_redis')


def load_config(path):
    with open(path) as f:
        return json.load(f)


def connect_redis(config, password=None):
    rcfg = config['redis']
    r = redis.Redis(host=rcfg['hostname'], port=rcfg['port'],
                    password=password or rcfg.get('password'),
                    socket_timeout=rcfg.get('timeout', 30))
    r.ping()
    return r


def decode(key):
    return key.decode() if isinstance(key, bytes) else key


def entry_ms(entry_id):
    return int(decode(entry_id).split('-')[0])


def next_id(entry_id):
    entry_id = decode(entry_id)
    ms, _, seq = entry_id.partition('-')
    return '%s-%d' % (ms, int(seq or 0) + 1)


def is_metric_stream(name):
    return any(name.endswith(s) for s in METRIC_SUFFIXES)


def is_archiver_internal(name):
    return name.startswith('archiver_')


def archiving_key(name):
    """Return the :archiving stream whose HWM governs this metric stream."""
    for suf in (':fast', ':slow', ':nearline'):
        if name.endswith(suf):
            return name[:-len(suf)] + ':archiving'
    if name.endswith(':archiving'):
        return name
    return None


def get_hwm(r, name):
    arch = archiving_key(name)
    if arch is None:
        return None
    hwm = r.get(arch + STATE_SUFFIX)
    if not hwm or hwm == b'0':
        return None
    return decode(hwm)


def scan_streams(r):
    """Yield metric and archiver-internal streams without scanning all DB keys."""
    seen = set()
    for pattern in ('*:archiving', '*:fast', '*:slow', '*:nearline'):
        for key in r.scan_iter(match=pattern, count=2000):
            name = decode(key)
            if name in seen:
                continue
            seen.add(name)
            yield key, name, r.xlen(key)

    for key in r.scan_iter(match='archiver_*', count=200):
        if r.type(key) != b'stream':
            continue
        name = decode(key)
        if name in seen:
            continue
        seen.add(name)
        yield key, name, r.xlen(key)


def plan_metric_trim(r, key, name, length, hwm, keep_fraction, min_len):
    """Plan a MINID trim that only removes already-archived entries."""
    # Count unarchived tail (typically small). Avoid xrange('-', hwm), which
    # pulls every archived entry and is prohibitively slow on large streams.
    unarchived = r.xrange(key, min=next_id(hwm), max='+')
    n_unarchived = len(unarchived)
    n_archived = length - n_unarchived
    if n_archived < min_len:
        return None, length, length, 'short_archived'

    keep_archived = max(1, int(n_archived * keep_fraction))
    if keep_archived >= n_archived:
        return None, length, length, 'nothing_to_trim'

    kept = r.xrevrange(key, max=hwm, min='-', count=keep_archived)
    if len(kept) < keep_archived:
        return None, length, length, 'short_archived'

    minid = decode(kept[-1][0])

    # Safety: never evict at or past the first unarchived entry.
    if entry_ms(minid) >= entry_ms(next_id(hwm)):
        return None, length, length, 'unsafe_minid'

    after = keep_archived + n_unarchived
    return ('minid', minid), length, after, None


def plan_internal_trim(length, keep_fraction, min_len):
    """Plan a MAXLEN trim for non-metric archiver status streams."""
    if length < min_len:
        return None, length, length, 'short'
    keep = max(1, int(length * keep_fraction))
    if keep >= length:
        return None, length, length, 'nothing_to_trim'
    return ('maxlen', keep), length, keep, None


def trim_streams(r, keep_fraction, min_len, batch_size, dry_run):
    stats = {
        'streams_trimmed': 0,
        'streams_skipped': 0,
        'skipped_no_hwm': 0,
        'skipped_unknown': 0,
        'entries_before': 0,
        'entries_after': 0,
    }
    batch = []
    scanned = 0
    t0 = time.time()

    for key, name, length in scan_streams(r):
        scanned += 1
        if scanned % 1000 == 0:
            log.info('scanned %d streams (%d trimmed, %d skipped, %.0fs)',
                     scanned, stats['streams_trimmed'],
                     stats['streams_skipped'], time.time() - t0)

        if is_archiver_internal(name):
            plan, before, after, reason = plan_internal_trim(
                length, keep_fraction, min_len)
        elif is_metric_stream(name):
            hwm = get_hwm(r, name)
            if hwm is None:
                stats['skipped_no_hwm'] += 1
                stats['streams_skipped'] += 1
                continue
            plan, before, after, reason = plan_metric_trim(
                r, key, name, length, hwm, keep_fraction, min_len)
        else:
            stats['skipped_unknown'] += 1
            stats['streams_skipped'] += 1
            continue

        if plan is None:
            stats['streams_skipped'] += 1
            continue

        stats['entries_before'] += before
        stats['entries_after'] += after
        batch.append((key, name, plan, before, after))

        if len(batch) < batch_size:
            continue
        stats['streams_trimmed'] += _flush_batch(r, batch, dry_run)
        batch = []

    if batch:
        stats['streams_trimmed'] += _flush_batch(r, batch, dry_run)

    stats['entries_removed'] = stats['entries_before'] - stats['entries_after']
    return stats


def _xtrim_supports_minid(client):
    """redis-py < 4 only exposes xtrim(name, maxlen); MINID needs execute_command."""
    try:
        return 'minid' in inspect.signature(client.xtrim).parameters
    except (TypeError, ValueError):
        return False


def _queue_xtrim(pipe, key, plan):
    mode, val = plan
    if mode == 'minid':
        if _xtrim_supports_minid(pipe):
            pipe.xtrim(key, minid=val, approximate=False)
        else:
            pipe.execute_command('XTRIM', key, 'MINID', val)
    elif _xtrim_supports_minid(pipe):
        pipe.xtrim(key, maxlen=val, approximate=False)
    else:
        pipe.xtrim(key, val, approximate=False)


def _flush_batch(r, batch, dry_run):
    if dry_run:
        for _key, name, plan, before, after in batch:
            mode, val = plan
            log.info('would trim %s: %d -> ~%d entries (%s %s)',
                     name, before, after, mode, val)
        return len(batch)

    pipe = r.pipeline(transaction=False)
    for key, _name, plan, _before, _after in batch:
        _queue_xtrim(pipe, key, plan)
    pipe.execute()
    return len(batch)


def memory_stats(r):
    info = r.info('memory')
    return {
        'used_memory': info.get('used_memory', 0),
        'used_memory_human': info.get('used_memory_human', '?'),
        'used_memory_peak_human': info.get('used_memory_peak_human', '?'),
    }


def main():
    ap = argparse.ArgumentParser(
        description='Trim older Redis stream entries that are already archived.')
    ap.add_argument('-c', '--config', default=DEFAULT_CONFIG)
    ap.add_argument('-pw', '--password', default=None,
                    help='Redis password (overrides config file)')
    ap.add_argument('--keep-fraction', type=float, default=0.5,
                    metavar='F', help='fraction of trim-eligible entries to '
                    'retain from the newest end (default: 0.5)')
    ap.add_argument('--min-len', type=int, default=2,
                    help='minimum archived entries (metric streams) or total '
                         'entries (internal streams) before trimming '
                         '(default: 2)')
    ap.add_argument('--batch-size', type=int, default=500,
                    help='pipeline batch size (default: 500)')
    ap.add_argument('--dry-run', action='store_true',
                    help='report planned trims without modifying Redis')
    ap.add_argument('-q', '--quiet', action='store_true')
    args = ap.parse_args()

    if not 0.0 < args.keep_fraction < 1.0:
        ap.error('--keep-fraction must be between 0 and 1 (exclusive)')

    logging.basicConfig(level=logging.WARNING if args.quiet else logging.INFO,
                        format='%(asctime)s %(levelname)s %(message)s',
                        datefmt='%Y-%m-%d %H:%M:%S')

    config = load_config(args.config)
    r = connect_redis(config, args.password)

    mem_before = memory_stats(r)
    log.info('Redis memory before: %s (peak %s)',
             mem_before['used_memory_human'],
             mem_before['used_memory_peak_human'])
    log.info('Database keys: %d', r.dbsize())

    t0 = time.time()
    stats = trim_streams(r, args.keep_fraction, args.min_len,
                         args.batch_size, args.dry_run)
    elapsed = time.time() - t0

    if not args.dry_run:
        try:
            r.memory_purge()
        except redis.ResponseError:
            pass

    mem_after = memory_stats(r)
    log.info('Done in %.1fs (%s)', elapsed,
             'dry-run' if args.dry_run else 'live')
    log.info('Streams trimmed: %d  skipped: %d  '
             '(no archive mark: %d, unknown type: %d)',
             stats['streams_trimmed'], stats['streams_skipped'],
             stats['skipped_no_hwm'], stats['skipped_unknown'])
    log.info('Stream entries: %d -> %d (removed ~%d)',
             stats['entries_before'], stats['entries_after'],
             stats['entries_removed'])
    if not args.dry_run:
        log.info('Redis memory after: %s (peak %s)',
                 mem_after['used_memory_human'],
                 mem_after['used_memory_peak_human'])
        freed = mem_before['used_memory'] - mem_after['used_memory']
        if freed > 0:
            log.info('Memory freed: %.2f MiB', freed / (1024 * 1024))


if __name__ == '__main__':
    main()
