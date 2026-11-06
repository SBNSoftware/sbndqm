#!/bin/bash
#
# Run the offline direct-to-PostgreSQL archiver workflow over every file listed
# in a filelist.
#
# The filelist is a plain text file with one input path per line. Blank lines
# and lines beginning with '#' are ignored.
#
# By default each file is processed individually (lar + DirectPgArchiver), with
# up to --jobs workers running in parallel (default: 10). This is restartable:
# re-run with --start to skip files already done. Use --batch to pass every
# file to one archive_offline_direct.sh invocation instead (no parallelism).
#
# Usage:
#   ./archive_offline_direct_filelist.sh filelist.txt
#   ./archive_offline_direct_filelist.sh filelist.txt --jobs 4
#   ./archive_offline_direct_filelist.sh filelist.txt --start 5
#   ./archive_offline_direct_filelist.sh filelist.txt --batch
#   ./archive_offline_direct_filelist.sh filelist.txt -- -- -gn tpc_channel
#
# Options:
#   --start N     1-based index of first file to process (default: 1)
#   --end N       1-based index of last file to process (default: last)
#   --jobs N      run up to N files in parallel (default: 10; use 1 for serial)
#   --log-dir DIR write worker logs here (default: temp dir under /tmp)
#   --batch       run all selected files in one archive_offline_direct.sh call
#   --dry-run     print planned commands without running them
#   --            remaining args are forwarded to archive_offline_direct.sh
#
# Environment:
#   ARCHIVER_JOBS     default parallel worker count (overridden by --jobs)
#   ARCHIVER_LOG_DIR  default log directory (overridden by --log-dir)
#   ARCHIVER_KEEP_LOGS=1  keep the log directory after exit
#   ARCHIVER_*        same remote staging vars as archive_offline_lib.sh
#
# Worker output goes to fixed log files in LOG_DIR (not stdout) to avoid pipe
# deadlocks. Each worker overwrites its log when it starts a new file, so logs
# do not accumulate across the run. Monitor progress:
#   tail -F /tmp/archiver_direct_filelist.XXXX/archiver.log        # --jobs 1
#   tail -F /tmp/archiver_direct_filelist.XXXX/worker_*.log       # --jobs N
#
# If a listed file is not found locally, archive_offline_direct.sh copies it
# directly from the remote host (default: munjung@sbndbuild03.fnal.gov:/pnfs/...).
#
# Before running (as munjung, before ksu sbnd):
#   kinit munjung@FNAL.GOV
#   ksu sbnd
#
# Examples:
#   ./archive_offline_direct_filelist.sh /data/my_run.files
#   ./archive_offline_direct_filelist.sh run18318.files --jobs 4
#   ./archive_offline_direct_filelist.sh run18318.files --start 10 --end 20
#   ./archive_offline_direct_filelist.sh run18318.files -- -- -gn tpc_channel -mn rms

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ARCHIVE_OFFLINE_DIRECT="$HERE/archive_offline_direct.sh"

FILELIST=""
START=1
END=0
JOBS="${ARCHIVER_JOBS:-10}"
LOG_DIR="${ARCHIVER_LOG_DIR:-}"
BATCH=0
DRY_RUN=0
ARCHIVER_ARGS=()
seen_sep=0

FAILED=0
PROCESSED=0
RUNNING=0
CLEANUP_LOG_DIR=0
declare -a SLOT_BUSY=()

init_worker_slots() {
    SLOT_BUSY=()
    local s
    for ((s = 0; s < JOBS; s++)); do
        SLOT_BUSY[$s]=0
    done
}

find_free_slot() {
    local s
    for ((s = 0; s < JOBS; s++)); do
        if [[ "${SLOT_BUSY[$s]}" -eq 0 ]]; then
            echo "$s"
            return 0
        fi
    done
    return 1
}

worker_logfile() {
    local slot="$1"
    if (( JOBS == 1 )); then
        echo "${LOG_DIR}/archiver.log"
    else
        echo "${LOG_DIR}/worker_${slot}.log"
    fi
}

usage() {
    sed -n '2,45p' "$0" >&2
    exit 1
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --start)
            START="${2:?missing value for --start}"
            shift 2
            ;;
        --end)
            END="${2:?missing value for --end}"
            shift 2
            ;;
        --jobs|-j)
            JOBS="${2:?missing value for --jobs}"
            shift 2
            ;;
        --log-dir)
            LOG_DIR="${2:?missing value for --log-dir}"
            shift 2
            ;;
        --batch)
            BATCH=1
            shift
            ;;
        --dry-run)
            DRY_RUN=1
            shift
            ;;
        --)
            seen_sep=1
            shift
            ;;
        -h|--help)
            usage
            ;;
        *)
            if [[ $seen_sep -eq 1 ]]; then
                ARCHIVER_ARGS+=("$1")
            elif [[ -z "$FILELIST" ]]; then
                FILELIST="$1"
            else
                echo "ERROR: unexpected argument: $1" >&2
                usage
            fi
            shift
            ;;
    esac
done

[[ -n "$FILELIST" ]] || usage
[[ -f "$FILELIST" ]] || { echo "ERROR: filelist not found: $FILELIST" >&2; exit 1; }
[[ -x "$ARCHIVE_OFFLINE_DIRECT" ]] || { echo "ERROR: archive_offline_direct.sh not found: $ARCHIVE_OFFLINE_DIRECT" >&2; exit 1; }
if ! [[ "$JOBS" =~ ^[1-9][0-9]*$ ]]; then
    echo "ERROR: --jobs must be a positive integer (got: $JOBS)" >&2
    exit 1
fi

# Read filelist into FILES[].
FILES=()
while IFS= read -r line || [[ -n "$line" ]]; do
    line="${line%%#*}"   # strip trailing comments
    line="${line#"${line%%[![:space:]]*}"}"  # ltrim
    line="${line%"${line##*[![:space:]]}"}"  # rtrim
    [[ -z "$line" ]] && continue
    FILES+=("$line")
done < "$FILELIST"

TOTAL=${#FILES[@]}
if [[ $TOTAL -eq 0 ]]; then
    echo "ERROR: no files found in $FILELIST" >&2
    exit 1
fi

if [[ $END -eq 0 || $END -gt $TOTAL ]]; then
    END=$TOTAL
fi
if [[ $START -lt 1 || $START -gt $END ]]; then
    echo "ERROR: invalid range --start $START --end $END (total files: $TOTAL)" >&2
    exit 1
fi

if [[ -z "$LOG_DIR" ]]; then
    LOG_DIR="$(mktemp -d "${TMPDIR:-/tmp}/archiver_direct_filelist.XXXXXX")"
    CLEANUP_LOG_DIR=1
else
    mkdir -p "$LOG_DIR"
fi

cleanup_logs() {
    if [[ $CLEANUP_LOG_DIR -eq 1 && "${ARCHIVER_KEEP_LOGS:-0}" != 1 ]]; then
        rm -rf "$LOG_DIR"
    fi
}
trap cleanup_logs EXIT

echo "=== Offline direct PostgreSQL archiver filelist ==="
echo "Filelist: $FILELIST"
echo "Files in list: $TOTAL"
echo "Processing: $START .. $END ($(($END - $START + 1)) files)"
if [[ $BATCH -eq 1 ]]; then
    echo "Mode: batch"
else
    echo "Mode: per-file (up to $JOBS parallel jobs)"
    echo "Logs: $LOG_DIR"
    if (( JOBS == 1 )); then
        echo "Monitor: tail -F ${LOG_DIR}/archiver.log"
    else
        echo "Monitor: tail -F ${LOG_DIR}/worker_*.log"
    fi
fi

run_archive_offline_direct() {
    if [[ $DRY_RUN -eq 1 ]]; then
        echo "DRY-RUN: $ARCHIVE_OFFLINE_DIRECT $*"
        return 0
    fi
    "$ARCHIVE_OFFLINE_DIRECT" "$@"
}

reap_one_job() {
    local pid status file idx logfile slot
    wait -n -p pid
    status=$?
    IFS='|' read -r idx file logfile slot <<< "${JOB_META[$pid]}"
    unset "JOB_META[$pid]"
    SLOT_BUSY[$slot]=0
    RUNNING=$((RUNNING - 1))
    if [[ $status -eq 0 ]]; then
        PROCESSED=$((PROCESSED + 1))
        echo "[$idx/$END] OK $file"
    else
        echo "ERROR: archive_offline_direct.sh failed on [$idx/$END] $file (exit $status)" >&2
        echo "       log: $logfile" >&2
        tail -20 "$logfile" >&2 || true
        FAILED=$((FAILED + 1))
    fi
}

if [[ $BATCH -eq 1 ]]; then
    BATCH_FILES=()
    for ((i = START - 1; i < END; i++)); do
        BATCH_FILES+=("${FILES[$i]}")
    done
    echo "=== Batch run: ${#BATCH_FILES[@]} files ==="
    if [[ ${#ARCHIVER_ARGS[@]} -gt 0 ]]; then
        run_archive_offline_direct "${BATCH_FILES[@]}" -- "${ARCHIVER_ARGS[@]}"
    else
        run_archive_offline_direct "${BATCH_FILES[@]}"
    fi
    exit $?
fi

declare -A JOB_META=()
init_worker_slots

for ((i = START; i <= END; i++)); do
    f="${FILES[$((i - 1))]}"
    while (( RUNNING >= JOBS )); do
        reap_one_job
    done

    if [[ $DRY_RUN -eq 1 ]]; then
        echo ""
        if [[ ${#ARCHIVER_ARGS[@]} -gt 0 ]]; then
            run_archive_offline_direct "$f" -- "${ARCHIVER_ARGS[@]}"
        else
            run_archive_offline_direct "$f"
        fi
        PROCESSED=$((PROCESSED + 1))
        continue
    fi

    slot="$(find_free_slot)"
    logfile="$(worker_logfile "$slot")"
    SLOT_BUSY[$slot]=1
    echo "[$i/$END] START $f -> $logfile"
    (
        echo "=== [$i/$END] START $(date -Is) $f ==="
        if [[ ${#ARCHIVER_ARGS[@]} -gt 0 ]]; then
            run_archive_offline_direct "$f" -- "${ARCHIVER_ARGS[@]}"
        else
            run_archive_offline_direct "$f"
        fi
        status=$?
        echo "=== [$i/$END] DONE $(date -Is) $f (exit $status) ==="
        exit "$status"
    ) > "$logfile" 2>&1 &
    pid=$!
    JOB_META[$pid]="$i|$f|$logfile|$slot"
    RUNNING=$((RUNNING + 1))
done

while (( RUNNING > 0 )); do
    reap_one_job
done
wait

echo ""
echo "=== Summary ==="
echo "Processed: $PROCESSED"
echo "Failed:    $FAILED"
echo "Logs:      $LOG_DIR"
[[ $FAILED -eq 0 ]]
