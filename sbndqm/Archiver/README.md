# SBND DQM Archiver

Archives online-monitoring metrics into the PostgreSQL run-conditions database
(`sbnd_online_prd` on `sbnd-db03`). There are two primary operating modes:

1. **Real-time (online)** — `Archiver.py` reads metrics from Redis streams as
   they are produced by the live DAQ / online DQM, and writes aggregated rows
   to PostgreSQL.
2. **File-list (offline)** — shell wrappers walk a text file of art ROOT paths,
   run offline DQM analysis on each file, and archive the resulting metrics.
   The preferred offline path writes **directly to PostgreSQL** with datataking
   timestamps (`archive_offline_direct*.sh`). An older Redis-hop path lives
   under `old_scripts/`.

Directory: `/home/nfs/sbnd/DQM_SPACK/dqm_env_2/sbndqm/sbndqm/Archiver`

---

## Directory structure

```
Archiver/
├── Archiver.py                      # Real-time / one-shot Redis → PostgreSQL archiver
├── ArchiverConfig.json              # Redis + PostgreSQL + logging config
├── requirements.txt                 # Python deps: redis, psycopg2-binary
├── fill_stream.py                   # Dev helper: inject random Redis stream data
├── flush_redis_old_half.py          # Trim archived Redis stream entries
├── find_pg_metric_gaps.py           # Report long gaps in PostgreSQL metric tables
│
├── archive_offline_lib.sh           # Shared remote staging (scp) helpers
├── archive_offline_direct.sh        # Offline: one/more ROOT files → JSONL → PG
├── archive_offline_direct_filelist.sh  # Offline: filelist → parallel direct archive
├── archive_offline_filelist.sh      # Offline: filelist → Redis path (needs archive_offline.sh)
│
├── filelists/                       # Example / production ROOT path lists
├── ddl/                             # SQL to create monitor tables / map
├── offline_direct/                  # Direct-to-PG art plugins + DirectPgArchiver
├── old_scripts/                     # Previous Redis-based offline wrappers
├── legacy/                          # Older Archiver.py
├── logs/                            # Worker logs from filelist runs
└── (runtime) archiver.log, gaps_*, offline_run*.log
```

### Purpose of each file

| Path | Role |
|------|------|
| `Archiver.py` | Main online archiver. Reads Redis `:archiving` streams, aggregates by monitor-map rules, inserts into PostgreSQL. Supports one-shot and `--daemon` modes. |
| `ArchiverConfig.json` | Connection settings for PostgreSQL (`metric_config` = monitor map), Redis, and log file location. |
| `requirements.txt` | `redis>=4.0`, `psycopg2-binary>=2.9`. |
| `fill_stream.py` | Test utility: continuously `XADD`s random values to named Redis streams. |
| `flush_redis_old_half.py` | Memory maintenance: trims metric streams only up to each stream’s `_LatestCompleted` high-water mark. |
| `find_pg_metric_gaps.py` | Scans metric tables for calendar gaps longer than a threshold (default 3 days). |
| `archive_offline_lib.sh` | Sourced by offline scripts. Resolves ROOT inputs locally or via `scp` |
| `archive_offline_direct.sh` | End-to-end offline job for explicit ROOT files: `lar` + `DirectPgArchiver.py` (no Redis). |
| `archive_offline_direct_filelist.sh` | Parallel driver over a filelist calling `archive_offline_direct.sh`. |
| `archive_offline_filelist.sh` | Parallel driver over a filelist calling `archive_offline.sh` (Redis path). **Note:** top-level `archive_offline.sh` is not present; use `old_scripts/archive_offline.sh` or the direct path. |
| `offline_direct/` | C++ art module + MetricPlugin that emit JSONL with datataking timestamps; `DirectPgArchiver.py` loads JSONL into PG. |
| `ddl/` | Schema helpers to create per-metric monitor tables and populate `monitor_map`. |
| `old_scripts/` | Prior versions of offline wrappers, including Redis-based `archive_offline.sh`. |
| `filelists/*.list` | One ROOT path per line (often under `/pnfs/...`). |
| `legacy/Archiver.py` | Historical archiver implementation. |

---

## Dependencies

**Python (online archiver and DirectPgArchiver):**

```bash
python3 -m pip install --user -r requirements.txt
# or: python3 -m venv env && source env/bin/activate && pip install -r requirements.txt
```

Packages: `redis`, `psycopg2` (via `psycopg2-binary`).

**Offline / file-list mode additionally needs:**

- DQM Spack environment (`setup_dqm_env.sh` / `offline_direct/setup_dqm_env_noninteractive.sh`)
- `lar` (art) with offline analysis FHiCL
- Built offline_direct plugins (`offline_direct/build_offline_direct.sh`) for the direct-PG path
- Network access to Redis/PostgreSQL on `sbnd-db03`

```bash
ksu sbnd
cd /home/nfs/sbnd/DQM_SPACK/dqm_env_2/sbndqm/sbndqm/Archiver
```

---

## Configuration

Default config: `ArchiverConfig.json` (override with `-c` / `ARCHIVER_CONFIG`).

```json
{
  "postgresql": {
    "database": "sbnd_online_prd",
    "hostname": "sbnd-db03",
    "port": 5434,
    "user": "runcon_admin",
    "password": "<secret>",
    "metric_config": "runcon_prd.monitor_map"
  },
  "redis": {
    "hostname": "sbnd-db03",
    "port": 6379,
    "password": "<secret>",
    "timeout": 30
  },
  "logging": {
    "directory": ".../Archiver",
    "name": "archiver.log",
    "overwrite": true
  }
}
```

The **monitor map** (`runcon_prd.monitor_map`) defines one row per
`(group_name, metric, channel_id)` with `monitor_type`, `postgres_table`, and
`time_interval`. Redis stream names are:

```text
<group_name>:<channel_id>:<metric>:archiving
```

Monitor types: `0=MEAN`, `1=MEDIAN`, `2=MODE`, `3=MAX`, `4=MIN`, `5=LAST`.

Useful environment variables for offline scripts:

| Variable | Default | Meaning |
|----------|---------|---------|
| `ARCHIVER_CONFIG` | `ArchiverConfig.json` | Config path |
| `ARCHIVER_REMOTE_HOST` | `sbndbuild03.fnal.gov` | Host for missing ROOT files |
| `ARCHIVER_REMOTE_USER` | `` | SSH/scp user |
| `ARCHIVER_LOCAL_STAGING` | `/data/sbnd_archiver_staging` | Local scp destination |
| `ARCHIVER_JOBS` | `10` | Default parallel workers for filelist scripts |
| `ARCHIVER_LOG_DIR` | temp under `/tmp` | Worker log directory |
| `ARCHIVER_KEEP_LOGS` | unset | Set to `1` to keep temp logs |
| `DQM_ENV` | noninteractive setup script | Spack/DQM environment |
| `OFFLINE_DIRECT_INSTALL` | `offline_direct/install` | Plugin install prefix |

---

## Mode 1: Real-time archiving (Redis → PostgreSQL)

### Flow

```text
Online DQM / DAQ metric plugins
        │  XADD to Redis streams
        ▼
Redis  <group>:<channel>:<metric>:archiving
        │
        │  Archiver.py (--daemon or cron one-shot)
        │    1. Load stream list from monitor_map
        │    2. Read entries newer than <stream>_LatestCompleted
        │    3. Aggregate into time_interval windows
        │    4. INSERT INTO runcon_prd.<table> ON CONFLICT DO NOTHING
        │    5. Advance high-water mark in Redis
        │    6. Publish archiver_last_entry_timestamp_ms (+ heartbeat in daemon)
        ▼
PostgreSQL  (channel_id, smpl_time, smpl_value)
```

### Triggers / when it runs

- **Daemon:** `python3 Archiver.py --daemon` — loops forever; default poll every 60 s; worker 0 emits `archiver_heartbeat`.
- **One-shot / cron:** `python3 Archiver.py` — archive everything new, then exit.
- Trailing incomplete windows are held for `--grace` seconds (default 60) unless `--flush` is set (used by offline Redis workflow).

### Components

- `Archiver.py` — workers (optional `-pr N` processes splitting streams)
- Redis — metric streams + `_LatestCompleted` state keys
- PostgreSQL — `monitor_map` + per-metric monitor tables
- `ArchiverConfig.json` — endpoints and credentials

### Command-line examples

```bash
cd /home/nfs/sbnd/DQM_SPACK/dqm_env_2/sbndqm/sbndqm/Archiver

# One-shot pass (cron-friendly)
python3 Archiver.py

# Continuous real-time daemon, 4 worker processes
python3 Archiver.py --daemon -pr 4

# Restrict groups / metrics
python3 Archiver.py --daemon -gn tpc_channel -mn rms
python3 Archiver.py -gn PMT -gn CRT_board

# Debug a single channel; archive every sample
python3 Archiver.py -ch 42 --interval-override 0 --flush

# Custom config / Redis password override
python3 Archiver.py -c ./ArchiverConfig.json -pw "$REDIS_PASSWORD"
```

`Archiver.py` options:

| Flag | Description |
|------|-------------|
| `-c/--config` | Config JSON path |
| `-pw/--password` | Redis password override |
| `-pr/--processes` | Worker process count |
| `-gn/--group` | Filter metric group (repeatable) |
| `-mn/--metric` | Filter metric name (repeatable) |
| `-ch/--channel` | Single channel id |
| `--daemon` | Run forever |
| `--poll-interval` | Seconds between daemon scans (default 60) |
| `--flush` | Include trailing incomplete window |
| `--grace` | Hold closed windows this many seconds (default 60) |
| `--interval-override` | Override map `time_interval`; `0` = every sample |
| `--chunk-size` | Streams per Redis pipeline batch (default 500) |
| `--quiet` | No console logging |

---

## Mode 2: File-list archiving (offline ROOT files)

### Input format

Plain text, one absolute ROOT path per line. Blank lines and `#` comments are
ignored. Example (`filelists/SBND_DATA_FullRun1Light_bnblight_Raw.list`):

```text
/pnfs/sbnd/archive/sbn/sbn_nd/data/raw/bnblight/.../data_EventBuilder6_....root
/pnfs/sbnd/archive/.../data_EventBuilder3_....root
# optional comment
```

If a path is missing locally, scripts stage it with:

```text
# TODO: this is Moon's example
scp munjung@sbndbuild03.fnal.gov:<path> → /data/sbnd_archiver_staging/...
```

(Requires a valid Kerberos ticket)

### Preferred flow: direct-to-PostgreSQL

```text
filelist.txt
    │
    ▼
archive_offline_direct_filelist.sh  (--jobs N parallel workers)
    │  per file:
    ▼
archive_offline_direct.sh
    │  1. source DQM env
    │  2. stage ROOT file if needed
    │  3. lar -c offline_analysis_direct_pg.fcl
    │       EventTimeRecorder + OfflineFileMetric → JSONL
    │       (smpl times = datataking event.time(), not wall clock)
    │  4. DirectPgArchiver.py JSONL → PostgreSQL
    │       (same monitor-map aggregation as Archiver.py)
    ▼
PostgreSQL
```

Why direct-PG: the older Redis offline path stamps Redis ingest (wall-clock)
IDs, which are wrong when `lar` replays faster than real time.

### Legacy flow: Redis hop (old_scripts)

```text
filelist → archive_offline_filelist.sh → archive_offline.sh
    → lar offline_analysis.fcl (metrics → Redis)
    → Archiver.py --flush (Redis → PostgreSQL)
```

Top-level `archive_offline_filelist.sh` still expects `./archive_offline.sh`.
That script currently lives only in `old_scripts/`. Prefer the direct path, or
copy/symlink `old_scripts/archive_offline.sh` to the Archiver root if you need
the Redis offline path.

### Components

| Component | Role |
|-----------|------|
| `archive_offline_direct_filelist.sh` | Parallel filelist driver |
| `archive_offline_direct.sh` | Per-file (or batch) lar + DirectPgArchiver |
| `offline_direct/offline_analysis_direct_pg.fcl` | FHiCL: offline analysis + JSONL metric plugins |
| `offline_direct/DirectPgArchiver.py` | JSONL → PG |
| `offline_direct/*.{cc,hh}` | EventTimeRecorder + OfflineFileMetric |
| `archive_offline_lib.sh` | Remote staging |
| `filelists/*.list` | Input path lists |

### Build offline_direct plugins (once)

```bash
export DQM_ENV=/home/nfs/sbnd/DQM_SPACK/dqm_env_2/sbndqm/sbndqm/Archiver/offline_direct/setup_dqm_env_noninteractive.sh
./offline_direct/build_offline_direct.sh
```

### Command-line examples

```bash
cd /home/nfs/sbnd/DQM_SPACK/dqm_env_2/sbndqm/sbndqm/Archiver

# Single ROOT file (direct PG)
./archive_offline_direct.sh /path/to/data.root
./archive_offline_direct.sh data.root -- -gn tpc_channel -mn rms

# Filelist, 4 parallel workers
./archive_offline_direct_filelist.sh filelists/SBND_DATA_FullRun1Light_bnblight_Raw.list --jobs 4

# Restartable range (1-based indices)
./archive_offline_direct_filelist.sh my_run.files --start 10 --end 20 --jobs 2

# Keep logs; dry-run
ARCHIVER_KEEP_LOGS=1 ./archive_offline_direct_filelist.sh my_run.files --log-dir ./logs --dry-run

# Forward Archiver filters after --
./archive_offline_direct_filelist.sh my_run.files -- -- -gn tpc_channel -mn rms

# Batch: one archive_offline_direct.sh call with all selected files
./archive_offline_direct_filelist.sh my_run.files --batch

# Inspect art event timestamps
python3 offline_direct/extract_art_event_times.py data.root --max-events 10
```

Filelist driver options (`archive_offline_direct_filelist.sh` /
`archive_offline_filelist.sh`):

| Option | Description |
|--------|-------------|
| `--start N` / `--end N` | 1-based inclusive file index range |
| `--jobs N` / `-j N` | Parallel workers (default 10) |
| `--log-dir DIR` | Worker log directory |
| `--batch` | One child invocation with all files |
| `--dry-run` | Print commands only |
| `--` | Remaining args forwarded to the per-file script |

Monitor progress:

```bash
tail -F /tmp/archiver_direct_filelist.XXXX/worker_*.log
```

---

## Maintenance utilities

```bash
# Preview Redis trim (safe)
python3 flush_redis_old_half.py --dry-run
python3 flush_redis_old_half.py --keep-fraction 0.25

# Find multi-day holes in the PG archive
python3 find_pg_metric_gaps.py -c ArchiverConfig.json

# Inject test stream data
python3 fill_stream.py -s 'tpc_channel:0:rms:archiving' -hn sbnd-db03 -pw "$REDIS_PASSWORD"
```

### Creating monitor tables (DDL)

```bash
cd ddl
./createMonitor.sh <group> <metric> <monitor_type 0..5> <ch_first> <ch_last>
# runs channelMonitor.sql via psql against sbnd_online_prd
```

---

## Quick reference: which entry point?

| Goal | Entry point |
|------|-------------|
| Live online archiving | `python3 Archiver.py --daemon` |
| Cron / catch-up from Redis | `python3 Archiver.py` |
| Offline one ROOT file (correct timestamps) | `./archive_offline_direct.sh file.root` |
| Offline many files from a list | `./archive_offline_direct_filelist.sh list.txt` |
| Offline via Redis (legacy) | `old_scripts/archive_offline.sh` (+ filelist wrapper) |
