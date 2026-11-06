#!/bin/bash
#
# Offline DQM workflow that writes archive metrics directly to PostgreSQL
# using datataking timestamps from art ROOT events (no Redis hop).
#
# Steps:
#   1. set up the DQM spack environment
#   2. run offline_analysis_direct_pg.fcl over an art ROOT file
#      (EventTimeRecorder + OfflineFileMetric via sendMetric)
#   3. run DirectPgArchiver.py on the JSONL output
#
# Usage:
#   ./archive_offline_direct.sh <input1.root> [input2.root ...] [-- <extra DirectPgArchiver args>]
#
# Environment:
#   DQM_ENV                 path to setup_dqm_env.sh
#   OFFLINE_DIRECT_INSTALL  plugin install prefix (default: offline_direct/install)
#   ARCHIVER_*              same as archive_offline_lib.sh

DQM_ENV="${DQM_ENV:-/home/nfs/sbnd/DQM_SPACK/dqm_env_2/sbndqm/sbndqm/Archiver/offline_direct/setup_dqm_env_noninteractive.sh}"
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OFFLINE_DIRECT="${HERE}/offline_direct"
OFFLINE_INSTALL="${OFFLINE_DIRECT_INSTALL:-/home/nfs/sbnd/DQM_SPACK/dqm_env_2/sbndqm/sbndqm/Archiver/offline_direct/install}"
FCL="${OFFLINE_DIRECT}/offline_analysis_direct_pg.fcl"
# shellcheck source=archive_offline_lib.sh
source "$HERE/archive_offline_lib.sh"

INPUTS=()
ARCHIVER_ARGS=()
seen_sep=0
for arg in "$@"; do
    if [[ "$arg" == "--" ]]; then seen_sep=1; continue; fi
    if [[ $seen_sep -eq 0 ]]; then INPUTS+=("$arg"); else ARCHIVER_ARGS+=("$arg"); fi
done

if [[ ${#INPUTS[@]} -eq 0 ]]; then
    echo "Usage: $0 <input1.root> [input2.root ...] [-- <extra DirectPgArchiver args>]" >&2
    exit 1
fi

if [[ ! -f "${OFFLINE_INSTALL}/lib/libsbndqm_offline_event_window.so" ]]; then
    echo "ERROR: offline direct plugins not built." >&2
    echo "       Run: ${OFFLINE_DIRECT}/build_offline_direct.sh" >&2
    exit 1
fi

echo "=== Setting up DQM environment ==="
shopt -s expand_aliases
if [[ -f "${DQM_ENV}" ]]; then
    source "${DQM_ENV}"
else
    DEV_ARCHIVER_ROOT="$(cd "$HERE/../.." && pwd)"
    if [[ -f "${DEV_ARCHIVER_ROOT}/setup.sh" ]]; then
        source "${DEV_ARCHIVER_ROOT}/setup.sh"
    else
        echo "ERROR: DQM environment not found at ${DQM_ENV}" >&2
        exit 1
    fi
fi

export FHICL_FILE_PATH="${OFFLINE_DIRECT}:${OFFLINE_INSTALL}/share/fcl:${FHICL_FILE_PATH:-}"
export CET_PLUGIN_PATH="${OFFLINE_INSTALL}/lib:${CET_PLUGIN_PATH:-}"
export ART_PLUGIN_PATH="${OFFLINE_INSTALL}/lib:${ART_PLUGIN_PATH:-}"
export LD_LIBRARY_PATH="${OFFLINE_INSTALL}/lib:${LD_LIBRARY_PATH:-}"

if ! command -v lar >/dev/null; then
    echo "ERROR: 'lar' not found after sourcing the DQM environment." >&2
    exit 1
fi
set -eo pipefail

export PYTHONPATH="${PYTHONPATH:-}${PYTHONPATH:+:}${HOME}/.local/lib/python3.9/site-packages"

PY=""
for cand in "$HERE/env/bin/python" /usr/bin/python3 python3; do
    if "$cand" -c 'import psycopg2' >/dev/null 2>&1; then
        PY="$cand"
        break
    fi
done
if [[ -z "$PY" ]]; then
    echo "ERROR: no python3 with psycopg2 found." >&2
    echo "       Run: python3 -m pip install --user -r $HERE/requirements.txt" >&2
    exit 1
fi

CONFIG="${ARCHIVER_CONFIG:-}"
if [[ -z "$CONFIG" ]]; then
    for cand in "$HERE/ArchiverConfig.json" /home/nfs/sbnddqm/ArchiverConfig.json; do
        if [[ -f "$cand" ]]; then
            CONFIG="$cand"
            break
        fi
    done
fi
if [[ -n "$CONFIG" ]]; then
    ARCHIVER_ARGS+=(-c "$CONFIG")
fi

write_metrics_fcl() {
    local jsonl="$1"
    local out="$2"
    cat > "$out" <<EOF
#include "offline_analysis_direct_pg.fcl"

services.InitMetricManager.connections.fast.output_path: "${jsonl}"
services.InitMetricManager.connections.slow.output_path: "${jsonl}"
services.InitMetricManager.connections.archive.output_path: "${jsonl}"
physics.analyzers.OnlineAnalysis.metrics.redis_metric_10s.output_path: "${jsonl}"
physics.analyzers.OnlineAnalysis.metrics.redis_metric_archive.output_path: "${jsonl}"
EOF
}

for f in "${INPUTS[@]}"; do
    stage_input_file "$f" || exit 1

    METRICS_JSONL="$(mktemp /tmp/sbnd_offline_metrics.XXXXXX.jsonl)"
    RUN_FCL="$(mktemp /tmp/offline_analysis_direct_run.XXXXXX.fcl)"
    write_metrics_fcl "$METRICS_JSONL" "$RUN_FCL"

    echo "=== Running offline analysis (metrics -> JSONL, datataking timestamps) ==="
    echo "--- lar -c $(basename "$RUN_FCL") -s ${RESOLVED_INPUT_PATH}"
    echo "--- metrics JSONL: ${METRICS_JSONL}"
    lar -c "$RUN_FCL" -s "${RESOLVED_INPUT_PATH}"
    lar_status=$?
    rm -f "$RUN_FCL"
    cleanup_staged_input
    if [[ $lar_status -ne 0 ]]; then
        rm -f "${METRICS_JSONL}"
        echo "ERROR: lar failed on ${f} (exit ${lar_status})" >&2
        exit "$lar_status"
    fi

    if [[ ! -s "${METRICS_JSONL}" ]]; then
        rm -f "${METRICS_JSONL}"
        echo "ERROR: no metrics written to ${METRICS_JSONL}" >&2
        exit 1
    fi

    echo "=== Archiving metrics directly to PostgreSQL ==="
    "$PY" "${OFFLINE_DIRECT}/DirectPgArchiver.py" "${METRICS_JSONL}" "${ARCHIVER_ARGS[@]}"
    rm -f "${METRICS_JSONL}"
done

echo "=== Done ==="
