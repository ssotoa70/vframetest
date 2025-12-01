#!/bin/bash
#
# Integration tests for vframetest
# Tests real filesystem I/O with actual storage
#

set -e

VFRAMETEST="${VFRAMETEST:-./build/vframetest}"
TEST_DIR="${TEST_DIR:-/tmp/vframetest_integration_$$}"
FRAMES="${FRAMES:-50}"
PASS=0
FAIL=0

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

cleanup() {
    rm -rf "$TEST_DIR"
}

trap cleanup EXIT

log_pass() {
    echo -e "${GREEN}PASS${NC}: $1"
    PASS=$((PASS + 1))
}

log_fail() {
    echo -e "${RED}FAIL${NC}: $1"
    FAIL=$((FAIL + 1))
}

log_info() {
    echo -e "${YELLOW}TEST${NC}: $1"
}

run_test() {
    local name="$1"
    shift
    local cmd="$@"

    log_info "$name"
    if eval "$cmd" > /dev/null 2>&1; then
        log_pass "$name"
        return 0
    else
        log_fail "$name"
        return 1
    fi
}

# Check vframetest exists
if [ ! -x "$VFRAMETEST" ]; then
    echo "Error: $VFRAMETEST not found or not executable"
    echo "Run 'make' first to build vframetest"
    exit 1
fi

echo "============================================"
echo "vframetest Integration Tests"
echo "============================================"
echo "Binary: $VFRAMETEST"
echo "Test dir: $TEST_DIR"
echo "Frames per test: $FRAMES"
echo ""

mkdir -p "$TEST_DIR"

# -------------------------------------------
# Basic functionality tests
# -------------------------------------------
echo "--- Basic Functionality ---"

run_test "Version check" \
    "$VFRAMETEST --version"

# --help returns exit 1 but should produce output
log_info "Help output"
if $VFRAMETEST --help 2>&1 | grep -q "Usage:"; then
    log_pass "Help output"
else
    log_fail "Help output"
fi

run_test "List profiles" \
    "$VFRAMETEST --list-profiles"

# -------------------------------------------
# Write tests - all profiles
# -------------------------------------------
echo ""
echo "--- Write Tests (all profiles) ---"

for profile in SD-32bit-cmp HD-24bit FULLHD-24bit 4K-24bit; do
    rm -rf "$TEST_DIR"/*
    run_test "Write $profile" \
        "$VFRAMETEST -w $profile -n $FRAMES $TEST_DIR"
done

# -------------------------------------------
# Write + Read tests
# -------------------------------------------
echo ""
echo "--- Write + Read Tests ---"

rm -rf "$TEST_DIR"/*
run_test "Write SD frames" \
    "$VFRAMETEST -w SD-32bit-cmp -n $FRAMES $TEST_DIR"

run_test "Read SD frames" \
    "$VFRAMETEST -r -n $FRAMES $TEST_DIR"

# -------------------------------------------
# Access patterns
# -------------------------------------------
echo ""
echo "--- Access Pattern Tests ---"

rm -rf "$TEST_DIR"/*
run_test "Write sequential" \
    "$VFRAMETEST -w SD-32bit-cmp -n $FRAMES $TEST_DIR"

rm -rf "$TEST_DIR"/*
run_test "Write reverse" \
    "$VFRAMETEST -w SD-32bit-cmp -n $FRAMES --reverse $TEST_DIR"

rm -rf "$TEST_DIR"/*
run_test "Write random" \
    "$VFRAMETEST -w SD-32bit-cmp -n $FRAMES --random $TEST_DIR"

# -------------------------------------------
# Streaming mode (single file)
# -------------------------------------------
echo ""
echo "--- Streaming Mode Tests ---"

rm -rf "$TEST_DIR"/*
touch "$TEST_DIR/stream.bin"
run_test "Streaming write" \
    "$VFRAMETEST -w SD-32bit-cmp -n $FRAMES -s $TEST_DIR/stream.bin"

# Frame size for SD-32bit-cmp: 720*480*4 = 1382400
run_test "Streaming read" \
    "$VFRAMETEST -r -n $FRAMES -z 1382400 -s $TEST_DIR/stream.bin"

# -------------------------------------------
# Output formats
# -------------------------------------------
echo ""
echo "--- Output Format Tests ---"

rm -rf "$TEST_DIR"/*

run_test "CSV output" \
    "$VFRAMETEST -w SD-32bit-cmp -n $FRAMES --csv $TEST_DIR"

rm -rf "$TEST_DIR"/*
run_test "JSON output" \
    "$VFRAMETEST -w SD-32bit-cmp -n $FRAMES --json $TEST_DIR"

rm -rf "$TEST_DIR"/*
run_test "Histogram output" \
    "$VFRAMETEST -w SD-32bit-cmp -n $FRAMES --histogram $TEST_DIR"

rm -rf "$TEST_DIR"/*
run_test "Times breakdown" \
    "$VFRAMETEST -w SD-32bit-cmp -n $FRAMES --times $TEST_DIR"

rm -rf "$TEST_DIR"/*
run_test "Frametimes output" \
    "$VFRAMETEST -w SD-32bit-cmp -n $FRAMES --frametimes $TEST_DIR"

# -------------------------------------------
# Threading tests
# -------------------------------------------
echo ""
echo "--- Threading Tests ---"

rm -rf "$TEST_DIR"/*
run_test "Single thread write" \
    "$VFRAMETEST -w SD-32bit-cmp -n $FRAMES -t 1 $TEST_DIR"

rm -rf "$TEST_DIR"/*
run_test "Multi-thread write (2)" \
    "$VFRAMETEST -w SD-32bit-cmp -n $FRAMES -t 2 $TEST_DIR"

rm -rf "$TEST_DIR"/*
run_test "Multi-thread write (4)" \
    "$VFRAMETEST -w SD-32bit-cmp -n $FRAMES -t 4 $TEST_DIR"

# -------------------------------------------
# FPS limiting
# -------------------------------------------
echo ""
echo "--- FPS Limiting Tests ---"

rm -rf "$TEST_DIR"/*
run_test "FPS limited write (60fps)" \
    "$VFRAMETEST -w SD-32bit-cmp -n 10 --fps 60 $TEST_DIR"

# -------------------------------------------
# Empty frames test
# -------------------------------------------
echo ""
echo "--- Empty Frames Test ---"

rm -rf "$TEST_DIR"/*
run_test "Empty frame write" \
    "$VFRAMETEST -e -w SD-32bit-cmp -n $FRAMES $TEST_DIR"

# -------------------------------------------
# Stress tests
# -------------------------------------------
echo ""
echo "--- Stress Tests ---"

rm -rf "$TEST_DIR"/*
run_test "Large frame count (200)" \
    "$VFRAMETEST -w SD-32bit-cmp -n 200 --histogram $TEST_DIR"

rm -rf "$TEST_DIR"/*
run_test "Large profile (4K)" \
    "$VFRAMETEST -w 4K-24bit -n 20 --histogram $TEST_DIR"

rm -rf "$TEST_DIR"/*
run_test "Combined options" \
    "$VFRAMETEST -w HD-24bit -n $FRAMES -t 2 --histogram --times --csv $TEST_DIR"

# -------------------------------------------
# Summary
# -------------------------------------------
echo ""
echo "============================================"
echo "Results: ${GREEN}$PASS passed${NC}, ${RED}$FAIL failed${NC}"
echo "============================================"

if [ $FAIL -gt 0 ]; then
    exit 1
fi

exit 0
