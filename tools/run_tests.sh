#!/usr/bin/env bash
# Build spellcraft_test.z64, run it in ares and print the test log.
#
# Usage: tools/run_tests.sh [-t SECONDS] [-g GDB_SCRIPT]
#
#   -t SECONDS     give up after this long (default 30)
#   -g GDB_SCRIPT  start ares paused with its GDB server on port 8080, then run
#                  gdb-multiarch in batch mode on build/test.elf with this
#                  script. The script should start with
#                  `target remote 127.0.0.1:8080`.
#
# Test output is written through the ISViewer by debugf() and ares echoes it
# to stdout. The run ends when main_test.c prints "TESTS DONE", when an
# exception is reported, or on timeout. Exits non-zero if any test failed or
# the run didn't finish.
set -euo pipefail

ROOT=$(cd "$(dirname "$0")/.." && pwd)
ARES=${ARES:-$HOME/github/ares/build/desktop-ui/ares}
GDB=${GDB:-gdb-multiarch}
GDB_PORT=8080
TIMEOUT=30
GDB_SCRIPT=

while getopts "t:g:" opt; do
    case $opt in
        t) TIMEOUT=$OPTARG ;;
        g) GDB_SCRIPT=$(realpath "$OPTARG") ;;
        *) echo "usage: $0 [-t SECONDS] [-g GDB_SCRIPT]" >&2; exit 2 ;;
    esac
done

cd "$ROOT"
make tests

LOG=$(mktemp)
ARES_ARGS=(--setting Input/Defocus=Allow)
if [ -n "$GDB_SCRIPT" ]; then
    ARES_ARGS+=(
        --setting Developer/DebugServerPort=$GDB_PORT
        --setting Developer/DebugServerEnabled=true
        --setting Developer/DebugServerUseIPv4=true
        --setting Boot/AwaitGDBClient=true
    )
fi

"$ARES" "${ARES_ARGS[@]}" spellcraft_test.z64 > "$LOG" 2>&1 &
ARES_PID=$!
cleanup() {
    local status=$?
    kill $ARES_PID 2>/dev/null && wait $ARES_PID 2>/dev/null || true
    rm -f "$LOG"
    exit $status
}
trap cleanup EXIT

if [ -n "$GDB_SCRIPT" ]; then
    for _ in $(seq 50); do
        grep -q "Opening TCP-server" "$LOG" && break
        sleep 0.2
    done
    # gdb blocks until its script finishes; the ROM runs while it's attached.
    timeout "$TIMEOUT" "$GDB" -q -batch -x "$GDB_SCRIPT" build/test.elf || true
fi

deadline=$(( SECONDS + TIMEOUT ))
while (( SECONDS < deadline )); do
    grep -qE "^TESTS DONE|^exception " "$LOG" && break
    kill -0 $ARES_PID 2>/dev/null || break
    sleep 0.2
done

# Only keep the ROM's own output, not ares/OpenGL/pipewire startup noise.
sed -n '/^Loaded spellcraft_test/,$p' "$LOG" | grep -v -e '^Vulkan' -e 'pw\.conf' -e '^Opening TCP' -e '^GDB client'

if ! grep -q "^TESTS DONE" "$LOG"; then
    echo "run_tests: tests did not finish (timeout ${TIMEOUT}s, crash, or gdb left the CPU halted)" >&2
    exit 1
fi
if grep -q "^TEST FAIL" "$LOG"; then
    exit 1
fi
