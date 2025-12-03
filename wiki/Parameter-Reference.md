# vframetest Parameter Reference Guide

**Version:** 25.17.2
**Last Updated:** December 2, 2025

Complete reference guide for all vframetest command-line parameters with detailed explanations and practical examples.

---

## Table of Contents

1. [Basic Syntax](#basic-syntax)
2. [Test Mode Parameters](#test-mode-parameters)
3. [Frame Profile Parameters](#frame-profile-parameters)
4. [Execution Control Parameters](#execution-control-parameters)
5. [Output Format Parameters](#output-format-parameters)
6. [Display & Monitoring Parameters](#display--monitoring-parameters)
7. [Advanced Parameters](#advanced-parameters)
8. [Performance Tuning Parameters](#performance-tuning-parameters)
9. [Information Parameters](#information-parameters)
10. [Real-World Usage Scenarios](#real-world-usage-scenarios)

---

## Basic Syntax

```bash
vframetest [OPTIONS] <test-directory>
```

**Required:**
- `<test-directory>` - Path where test files will be written (write mode) or read from (read mode)

**Example:**
```bash
vframetest -w 4K-24bit -t 4 -n 100 /mnt/storage
```

---

## Test Mode Parameters

### `-w, --write [PROFILE]`

**Purpose:** Perform write tests with specified frame profile

**Usage:**
```bash
vframetest -w FULLHD-24bit -n 100 /tmp
vframetest -w 4K-32bit -t 8 -n 1000 /mnt/storage
```

**Details:**
- Primary test mode for measuring write performance
- Writes frames sequentially to test directory
- Measures throughput (MiB/s), FPS, and latency
- Can be combined with profile names (SD, HD, FULLHD, 4K, 8K)

**Common Profiles:**
- `SD-24bit` - 720x576 (Standard Definition)
- `HD-24bit` - 1280x720 (720p)
- `FULLHD-24bit` - 1920x1080 (1080p)
- `4K-24bit` - 3840x2160 (4K UHD)
- `8K-24bit` - 7680x4320 (8K UHD)
- `4K-32bit` - 3840x2160 with 32-bit color depth

**When to Use:**
- Testing storage write performance
- Validating video capture workflows
- Benchmarking NAS/SAN write speeds

---

### `-r, --read`

**Purpose:** Perform read tests on previously written frames

**Usage:**
```bash
# First write frames
vframetest -w FULLHD-24bit -n 100 /tmp

# Then read them back
vframetest -r -z 1920x1080x3 -n 100 /tmp
```

**Details:**
- Reads frames previously written by vframetest
- Requires `-z` parameter to specify frame dimensions
- Measures read throughput and latency
- Validates frame integrity

**When to Use:**
- Testing storage read performance
- Validating video playback workflows
- Verifying data integrity after write

---

### `-e, --empty`

**Purpose:** Perform write tests with empty (zero-filled) frames

**Usage:**
```bash
vframetest -e -w FULLHD-24bit -n 100 /tmp
```

**Details:**
- Writes frames filled with zeros
- Faster than writing real frame data
- Tests pure I/O performance without data generation overhead
- Useful for isolating storage bottlenecks

**When to Use:**
- Pure I/O benchmarking
- Testing compression-capable storage (zeros compress well)
- Isolating CPU vs storage bottlenecks

---

### `-s, --streaming [FILE]`

**Purpose:** Perform streaming test to a single file

**Usage:**
```bash
vframetest -s output.raw -w FULLHD-24bit -n 100 /tmp
```

**Details:**
- Writes all frames to a single continuous file
- Simulates video capture to a single file stream
- Different I/O pattern than per-frame files
- Requires `-z` parameter for read operations

**When to Use:**
- Testing continuous streaming workflows
- Simulating video recorder behavior
- Measuring sequential write performance

---

## Frame Profile Parameters

### `-z, --frame-size [WxHxD]`

**Purpose:** Specify custom frame dimensions

**Format:** `WIDTH x HEIGHT x DEPTH`

**Usage:**
```bash
# Custom resolution: 2560x1440 with 3 bytes per pixel (24-bit)
vframetest -e -z 2560x1440x3 -n 100 /tmp

# Required for reading frames
vframetest -r -z 1920x1080x3 -n 100 /tmp

# 32-bit color depth (4 bytes per pixel)
vframetest -e -z 3840x2160x4 -n 100 /tmp
```

**Details:**
- WIDTH: Horizontal resolution in pixels
- HEIGHT: Vertical resolution in pixels
- DEPTH: Bytes per pixel (3 for 24-bit, 4 for 32-bit)
- Required when using `-r` (read mode)
- Required when using `-s` (streaming mode)

**Common Depths:**
- `3` - 24-bit RGB (8 bits per channel)
- `4` - 32-bit RGBA or 10-bit RGB packed

**When to Use:**
- Testing non-standard resolutions
- Read operations
- Streaming mode
- Custom frame formats

---

### `-l, --list-profiles`

**Purpose:** List all available frame profiles

**Usage:**
```bash
vframetest --list-profiles
```

**Output Example:**
```
Available profiles:
  SD-24bit          (720x576x3)    - 1.24 MB per frame
  HD-24bit          (1280x720x3)   - 2.76 MB per frame
  FULLHD-24bit      (1920x1080x3)  - 6.22 MB per frame
  4K-24bit          (3840x2160x3)  - 24.88 MB per frame
  8K-24bit          (7680x4320x3)  - 99.53 MB per frame
  ...
```

**When to Use:**
- Discovering available profiles
- Checking frame sizes before testing
- Planning storage capacity requirements

---

### `--list-profiles-filter [PREFIX]`

**Purpose:** Filter profile list by name prefix

**Usage:**
```bash
# Show only 4K profiles
vframetest --list-profiles-filter 4K

# Show only 8K profiles
vframetest --list-profiles-filter 8K

# Show only HD variants
vframetest --list-profiles-filter HD
```

**When to Use:**
- Finding specific resolution profiles
- Narrowing down profile choices
- Scripting with specific profile types

---

## Execution Control Parameters

### `-t, --threads [COUNT]`

**Purpose:** Set number of concurrent threads for parallel testing

**Default:** 1

**Usage:**
```bash
# Single thread (baseline)
vframetest -w FULLHD-24bit -t 1 -n 100 /tmp

# 4 threads (moderate parallelism)
vframetest -w FULLHD-24bit -t 4 -n 100 /tmp

# 16 threads (high parallelism)
vframetest -w FULLHD-24bit -t 16 -n 100 /tmp
```

**Details:**
- Each thread writes frames independently
- Tests storage scalability under parallel load
- Higher thread counts reveal contention and bottlenecks
- Typical range: 1-16 threads

**Performance Impact:**
- More threads = higher total throughput (until saturation)
- Reveals storage system concurrency limits
- Network storage often shows poor scaling

**When to Use:**
- Testing multi-stream video workflows
- Finding optimal thread count for your storage
- Measuring storage scalability
- Simulating concurrent video capture/playback

**Best Practices:**
- Start with 1 thread for baseline
- Test 2, 4, 8, 16 threads to find scaling limits
- Match thread count to actual workflow needs

---

### `-n, --num-frames [COUNT]`

**Purpose:** Number of frames to write or read

**Default:** 1800 (1 minute at 30fps)

**Usage:**
```bash
# Short test (10 frames)
vframetest -w FULLHD-24bit -n 10 /tmp

# Standard test (100 frames)
vframetest -w FULLHD-24bit -n 100 /tmp

# Long test (10000 frames)
vframetest -w FULLHD-24bit -n 10000 /tmp
```

**Details:**
- Each thread writes this many frames
- Total frames = threads × num-frames
- More frames = more accurate average metrics
- More frames = longer test duration

**Recommended Values:**
- **Quick test:** 10-50 frames (5-10 seconds)
- **Standard test:** 100-500 frames (reliable averages)
- **Comprehensive test:** 1000-10000 frames (stress testing)

**Storage Requirements:**
```
Total Size = threads × num-frames × frame-size

Example: 4 threads × 100 frames × 6.2 MB (FULLHD) = 2.48 GB
```

**When to Use:**
- Quick validation: 10-50 frames
- Performance testing: 100-500 frames
- Stress testing: 1000+ frames
- Storage capacity validation: 10000+ frames

---

### `-f, --fps [RATE]`

**Purpose:** Limit frame rate to specific frames per second

**Usage:**
```bash
# Limit to 30 fps
vframetest -w FULLHD-24bit -f 30 -n 100 /tmp

# Limit to 60 fps
vframetest -w FULLHD-24bit -f 60 -n 100 /tmp

# Limit to 24 fps (cinema rate)
vframetest -w FULLHD-24bit -f 24 -n 100 /tmp
```

**Details:**
- Throttles write/read rate to specific FPS
- Simulates real-time video capture/playback
- Tests if storage can sustain required frame rate
- Each thread respects the FPS limit independently

**When to Use:**
- Validating real-time capture capabilities
- Testing if storage sustains specific frame rates
- Simulating camera recording workflows
- Ensuring playback can meet timeline requirements

**Example Scenarios:**
```bash
# Can storage handle 4K at 60fps with 4 streams?
vframetest -w 4K-24bit -f 60 -t 4 -n 1000 /mnt/storage

# Test cinema 8K at 24fps
vframetest -w 8K-24bit -f 24 -t 1 -n 500 /mnt/storage
```

---

### `-v, --reverse`

**Purpose:** Access files in reverse order

**Usage:**
```bash
# Write frames 100 -> 1
vframetest -w FULLHD-24bit -v -n 100 /tmp

# Read frames in reverse
vframetest -r -v -z 1920x1080x3 -n 100 /tmp
```

**Details:**
- Reverses frame access order
- Tests storage behavior with reverse sequential access
- Different I/O pattern than forward sequential
- Some storage systems optimize for forward access only

**When to Use:**
- Testing reverse video playback scenarios
- Validating storage handles reverse I/O well
- Identifying storage optimization biases
- Timeline scrubbing simulations

---

### `-m, --random`

**Purpose:** Access files in random order

**Usage:**
```bash
# Write frames in random order
vframetest -w FULLHD-24bit -m -n 100 /tmp

# Read frames randomly
vframetest -r -m -z 1920x1080x3 -n 100 /tmp
```

**Details:**
- Randomizes frame access order
- Simulates non-linear editing workflows
- Much harder on storage than sequential access
- Reveals worst-case latency and throughput

**When to Use:**
- Testing non-linear editing workflows
- Simulating timeline scrubbing
- Finding worst-case performance
- Validating random access performance

**Performance Impact:**
- Typically 2-10x slower than sequential access
- Network storage shows much worse random performance
- SSDs handle random I/O better than HDDs

---

## Output Format Parameters

### `-c, --csv`

**Purpose:** Output results in CSV format

**Usage:**
```bash
# Basic CSV output
vframetest -c -w FULLHD-24bit -n 100 /tmp

# Save to file
vframetest -c -w FULLHD-24bit -n 100 /tmp > results.csv

# Multiple tests to same file
vframetest -c --no-csv-header -w FULLHD-24bit -t 2 -n 100 /tmp >> results.csv
vframetest -c --no-csv-header -w FULLHD-24bit -t 4 -n 100 /tmp >> results.csv
vframetest -c --no-csv-header -w FULLHD-24bit -t 8 -n 100 /tmp >> results.csv
```

**Output Format:**
```csv
threads,frames,bytes,time_ns,fps,bytes_per_sec,mib_per_sec,success_rate
4,100,622873600,945123000,105.8,659234567,628.5,100.00
```

**When to Use:**
- Importing results into spreadsheets
- Automated test analysis
- Graphing performance trends
- Comparing multiple test runs

---

### `-j, --json`

**Purpose:** Output results in JSON format

**Usage:**
```bash
# JSON output
vframetest -j -w FULLHD-24bit -n 100 /tmp

# Save to file
vframetest -j -w FULLHD-24bit -n 100 /tmp > results.json

# Pretty-printed
vframetest -j -w FULLHD-24bit -n 100 /tmp | jq .
```

**Output Format:**
```json
{
  "profile": "FULLHD-24bit",
  "threads": 4,
  "frames": 100,
  "bytes_written": 622873600,
  "time_ns": 945123000,
  "fps": 105.8,
  "bytes_per_sec": 659234567,
  "mib_per_sec": 628.5,
  "success_rate": 100.0,
  "frames_succeeded": 100,
  "frames_failed": 0,
  "filesystem": "LOCAL"
}
```

**When to Use:**
- Programmatic result processing
- API integrations
- Automated CI/CD pipelines
- Database ingestion

---

### `--no-csv-header`

**Purpose:** Omit CSV header row

**Usage:**
```bash
# First test with header
vframetest -c -w FULLHD-24bit -t 1 -n 100 /tmp > results.csv

# Append more tests without duplicate headers
vframetest -c --no-csv-header -w FULLHD-24bit -t 2 -n 100 /tmp >> results.csv
vframetest -c --no-csv-header -w FULLHD-24bit -t 4 -n 100 /tmp >> results.csv
vframetest -c --no-csv-header -w FULLHD-24bit -t 8 -n 100 /tmp >> results.csv
```

**When to Use:**
- Appending to existing CSV files
- Collecting results from multiple runs
- Batch testing scripts

---

### `--times`

**Purpose:** Show detailed timing breakdown for each operation

**Usage:**
```bash
vframetest --times -w FULLHD-24bit -n 100 /tmp
```

**Output Example:**
```
Completion times:
 min   : 3.245 ms
 avg   : 5.678 ms
 max   : 12.345 ms

Operation breakdown:
 open  : avg 0.123 ms
 io    : avg 4.890 ms
 close : avg 0.665 ms
```

**Details:**
- Shows open, I/O, and close times separately
- Helps identify bottleneck phase
- Reveals filesystem overhead

**When to Use:**
- Diagnosing performance bottlenecks
- Identifying slow open/close operations
- Analyzing filesystem overhead
- Debugging performance issues

---

### `--frametimes`

**Purpose:** Output detailed timing for every individual frame in CSV format

**Usage:**
```bash
vframetest --frametimes -w FULLHD-24bit -n 100 /tmp > frametimes.csv
```

**Output Format:**
```csv
frame,thread,open_ns,io_ns,close_ns,total_ns
0,0,125000,4890000,665000,5680000
1,0,118000,5012000,623000,5753000
2,1,132000,4756000,688000,5576000
...
```

**When to Use:**
- Analyzing per-frame performance
- Identifying performance outliers
- Creating detailed performance graphs
- Statistical analysis of frame latency

**Analysis Use Cases:**
- Plotting latency distributions
- Finding performance anomalies
- Correlating frame timing with system events
- Creating percentile charts (P50, P95, P99)

---

### `--histogram`

**Purpose:** Show histogram of completion times at end of test

**Usage:**
```bash
vframetest --histogram -w FULLHD-24bit -n 1000 /tmp
```

**Output Example:**
```
Latency Histogram:
  0- 5ms: ████████████████████████████ (280 frames)
  5-10ms: ████████████████████████████████████████ (400 frames)
 10-15ms: ████████████ (120 frames)
 15-20ms: ████████ (80 frames)
 20-25ms: ████ (40 frames)
 25-30ms: ██ (20 frames)
 30+ms:   ████ (60 frames)
```

**When to Use:**
- Visualizing latency distribution
- Identifying performance consistency
- Spotting outliers and tail latency
- Quick visual performance assessment

---

## Display & Monitoring Parameters

### `--tui`

**Purpose:** Show real-time TUI (Terminal User Interface) dashboard during test

**Usage:**
```bash
vframetest --tui -w 8K-24bit -n 1000 -t 16 /tmp
```

**Features:**
- **Real-time progress bar** - Visual completion status
- **Live performance metrics** - Throughput, FPS, latency
- **Section-organized display:**
  - PROGRESS - Frame completion and ETA
  - PERFORMANCE METRICS - Throughput, FPS, data rates
  - LATENCY ANALYSIS - Min/avg/max times, percentiles
  - I/O MODE DISTRIBUTION - Direct vs buffered I/O
  - TEST RESULTS - Success/failure rates
  - FILESYSTEM & OPTIMIZATION - FS type and optimization status
  - SYSTEM STATUS - Overall health indicators

**Keyboard Controls:**
- `q` - Quit test
- `?` - Show help
- `SPACE` - Pause/resume
- `r` - Refresh display
- `s` - Show statistics

**When to Use:**
- Long-running tests (1000+ frames)
- Visual monitoring of test progress
- Real-time performance validation
- Identifying performance degradation during test

**Requirements:**
- Terminal with TTY support
- Minimum 80x24 terminal size
- ANSI color support (optional but recommended)

---

### `-i, --interactive`

**Purpose:** Launch interactive TTY mode with configuration menu

**Usage:**
```bash
vframetest -i /tmp
```

**Features:**
- **Interactive configuration menu**
  - Select frame profile from list
  - Configure thread count
  - Set frame count
  - Choose test mode (write/read/streaming)
- **Live test execution** - Built-in TUI during tests
- **Result history** - View previous test results
- **Easy re-testing** - Quickly run tests with different parameters

**Navigation:**
- Arrow keys - Navigate menu
- Enter - Select option
- Tab - Switch between sections
- Esc - Go back / Cancel
- q - Quit

**When to Use:**
- Exploratory testing without memorizing CLI flags
- Quick parameter adjustments
- New users learning vframetest
- Rapid iteration on test configurations

---

### `--history-size [COUNT]`

**Purpose:** Set frame history depth for interactive mode

**Default:** 10000

**Usage:**
```bash
vframetest -i --history-size 50000 /tmp
```

**Details:**
- Controls how many frames of history are kept in memory
- Higher values = more memory usage
- Affects graph detail in interactive mode
- Only relevant when using `-i` (interactive mode)

**Memory Impact:**
```
Memory ≈ history-size × 200 bytes

10000 frames ≈ 2 MB
50000 frames ≈ 10 MB
100000 frames ≈ 20 MB
```

**When to Use:**
- Long tests with detailed history
- Analyzing frame timing trends
- Creating detailed performance graphs

---

## Advanced Parameters

### `--header [SIZE]`

**Purpose:** Set frame header size (metadata before frame data)

**Default:** 65536 (64KB)

**Usage:**
```bash
# 128KB header
vframetest --header 131072 -w FULLHD-24bit -n 100 /tmp

# No header (raw frames)
vframetest --header 0 -w FULLHD-24bit -n 100 /tmp
```

**Details:**
- Header stores frame metadata
- Larger headers = more metadata space
- Header size affects total frame size
- Some formats require specific header sizes

**When to Use:**
- Matching specific video format requirements
- Testing with custom frame formats
- Simulating formats with large metadata
- Minimizing overhead with --header 0

---

## Performance Tuning Parameters

### `--no-metrics`

**Purpose:** Disable real-time metrics collection for maximum performance

**Usage:**
```bash
vframetest --no-metrics -w 8K-24bit -n 10000 -t 16 /tmp
```

**Details:**
- Disables live metrics tracking
- Reduces CPU overhead during testing
- Final results still reported at end
- Cannot be used with `--tui` (conflicts)

**Performance Impact:**
- 5-10% faster throughput on high-thread tests
- Lower CPU usage during test
- More accurate storage-only benchmarking

**When to Use:**
- Maximum performance benchmarking
- Reducing test overhead
- CPU-constrained systems
- Pure storage testing (no monitoring overhead)

**Trade-offs:**
- No real-time progress monitoring
- Cannot use with TUI
- Less visibility during test execution

---

## Information Parameters

### `-V, --version`

**Purpose:** Display version information

**Usage:**
```bash
vframetest --version
```

**Output:**
```
vframetest 25.17.2
```

---

### `-h, --help`

**Purpose:** Display help message with all parameters

**Usage:**
```bash
vframetest --help
```

---

## Real-World Usage Scenarios

### Scenario 1: Validating 4K Video Capture Storage

**Goal:** Ensure storage can handle 4K 60fps capture from 4 cameras

```bash
# Test 4K at 60fps with 4 threads (4 cameras)
vframetest --tui -w 4K-24bit -f 60 -t 4 -n 3600 /mnt/capture

# Expected minimum: ~6 GB/s (4 cameras × 24.88 MB/frame × 60 fps)
# Success criteria: 100% success rate, no frames > 16.67ms (60fps deadline)
```

**Validation:**
- Check success rate = 100%
- Verify average throughput ≥ 6000 MiB/s
- Confirm max latency < 16.67ms (60fps frame time)

---

### Scenario 2: NAS/SAN Performance Testing

**Goal:** Measure network storage performance and scalability

```bash
# Baseline: Single thread
vframetest -c -w FULLHD-24bit -t 1 -n 500 //nas-server/share > results.csv

# Scale testing
for threads in 2 4 8 16; do
  vframetest -c --no-csv-header -w FULLHD-24bit -t $threads -n 500 //nas-server/share >> results.csv
done

# Analyze scaling efficiency
cat results.csv
```

**Analysis:**
- Compare throughput increase vs thread count
- Identify saturation point
- Check if network storage scales linearly

---

### Scenario 3: Storage Qualification for 8K Post-Production

**Goal:** Validate storage meets 8K editing requirements

```bash
# Sequential write test (ingest)
vframetest --tui --histogram -w 8K-24bit -t 1 -n 1000 /mnt/storage

# Random access test (editing/scrubbing)
vframetest --tui --histogram -w 8K-24bit -m -t 4 -n 500 /mnt/storage

# Multi-stream playback test
vframetest --tui -w 8K-24bit -t 8 -n 1000 /mnt/storage
```

**Success Criteria:**
- Sequential: ≥ 2500 MiB/s (1 stream at 24fps)
- Random: ≥ 1000 MiB/s (acceptable scrubbing)
- Multi-stream: ≥ 5000 MiB/s (2-3 concurrent timelines)

---

### Scenario 4: Continuous Storage Monitoring

**Goal:** Monitor storage health over time

```bash
#!/bin/bash
# Daily storage validation
while true; do
  timestamp=$(date +%Y%m%d_%H%M%S)
  vframetest -j -w 4K-24bit -t 4 -n 100 /mnt/storage > "results_${timestamp}.json"

  # Extract throughput
  throughput=$(jq -r '.mib_per_sec' "results_${timestamp}.json")

  # Alert if below threshold
  if (( $(echo "$throughput < 2000" | bc -l) )); then
    echo "ALERT: Storage performance degraded to ${throughput} MiB/s"
    # Send alert notification
  fi

  sleep 86400  # Wait 24 hours
done
```

---

### Scenario 5: Performance Regression Testing

**Goal:** Detect storage or system performance changes

```bash
# Establish baseline
vframetest --frametimes -w FULLHD-24bit -t 4 -n 1000 /mnt/storage > baseline.csv

# After system changes
vframetest --frametimes -w FULLHD-24bit -t 4 -n 1000 /mnt/storage > current.csv

# Statistical comparison (using R, Python, or other tools)
# Compare P50, P95, P99 latencies
# Check for throughput regression > 10%
```

---

### Scenario 6: Comparing Storage Systems

**Goal:** Compare multiple storage solutions

```bash
# Test storage A
vframetest -c -w 4K-24bit -t 8 -n 500 /mnt/storage-a > storage-a.csv

# Test storage B
vframetest -c -w 4K-24bit -t 8 -n 500 /mnt/storage-b > storage-b.csv

# Test storage C (network)
vframetest -c -w 4K-24bit -t 8 -n 500 //nas/share > storage-c.csv

# Compare results
paste storage-a.csv storage-b.csv storage-c.csv | column -t
```

---

### Scenario 7: Worst-Case Performance Testing

**Goal:** Find performance floor under stress conditions

```bash
# Maximum stress: 8K, 16 threads, random access
vframetest --tui --histogram -w 8K-24bit -m -t 16 -n 1000 /mnt/storage

# Analyze histogram for tail latency
# Check P99 latency (99th percentile)
# Verify minimum acceptable throughput
```

---

### Scenario 8: CI/CD Storage Validation

**Goal:** Automated storage testing in deployment pipeline

```bash
#!/bin/bash
# CI/CD storage validation script

# Quick smoke test
vframetest -j -w FULLHD-24bit -n 50 /mnt/storage > test-result.json

# Parse results
success_rate=$(jq -r '.success_rate' test-result.json)
throughput=$(jq -r '.mib_per_sec' test-result.json)

# Validate thresholds
if (( $(echo "$success_rate < 100" | bc -l) )); then
  echo "FAIL: Success rate ${success_rate}% < 100%"
  exit 1
fi

if (( $(echo "$throughput < 500" | bc -l) )); then
  echo "FAIL: Throughput ${throughput} MiB/s < 500 MiB/s"
  exit 1
fi

echo "PASS: Storage validation successful"
exit 0
```

---

## Quick Reference Table

| Parameter | Short | Purpose | Common Value |
|-----------|-------|---------|--------------|
| `--write` | `-w` | Write test mode | `FULLHD-24bit`, `4K-24bit` |
| `--read` | `-r` | Read test mode | (flag only) |
| `--empty` | `-e` | Write empty frames | (flag only) |
| `--streaming` | `-s` | Stream to single file | `output.raw` |
| `--frame-size` | `-z` | Custom frame size | `1920x1080x3` |
| `--list-profiles` | `-l` | List profiles | (flag only) |
| `--threads` | `-t` | Thread count | `1`, `4`, `8`, `16` |
| `--num-frames` | `-n` | Frame count | `100`, `1000` |
| `--fps` | `-f` | Frame rate limit | `24`, `30`, `60` |
| `--reverse` | `-v` | Reverse access | (flag only) |
| `--random` | `-m` | Random access | (flag only) |
| `--csv` | `-c` | CSV output | (flag only) |
| `--json` | `-j` | JSON output | (flag only) |
| `--no-csv-header` | | Omit CSV header | (flag only) |
| `--times` | | Show timing breakdown | (flag only) |
| `--frametimes` | | Per-frame CSV timings | (flag only) |
| `--histogram` | | Show latency histogram | (flag only) |
| `--tui` | | Real-time TUI dashboard | (flag only) |
| `--interactive` | `-i` | Interactive mode | (flag only) |
| `--no-metrics` | | Disable metrics (faster) | (flag only) |
| `--header` | | Frame header size | `65536` (64KB) |
| `--history-size` | | Interactive history depth | `10000` |
| `--version` | `-V` | Show version | (flag only) |
| `--help` | `-h` | Show help | (flag only) |

---

## Parameter Combinations

### Common Workflows

**Basic Write Test:**
```bash
vframetest -w FULLHD-24bit -t 4 -n 100 /tmp
```

**Comprehensive Test with Monitoring:**
```bash
vframetest --tui --histogram --times -w 4K-24bit -t 8 -n 1000 /mnt/storage
```

**Batch Testing for Analysis:**
```bash
vframetest -c --frametimes -w 8K-24bit -t 16 -n 500 /mnt/storage > results.csv
```

**Interactive Exploration:**
```bash
vframetest -i /mnt/storage
```

**Performance Benchmarking (no overhead):**
```bash
vframetest --no-metrics -e -w 8K-24bit -t 16 -n 5000 /mnt/storage
```

---

## Best Practices

1. **Start Simple:** Begin with basic parameters, add complexity as needed
2. **Test Multiple Times:** Run tests 3-5 times and average results
3. **Use Appropriate Frame Counts:** 100 frames minimum for reliable averages
4. **Monitor with TUI:** Use `--tui` for long-running tests
5. **Export Data:** Use `-c` or `-j` for analysis and reporting
6. **Check Success Rate:** Always verify 100% success rate before trusting throughput numbers
7. **Consider Thread Count:** Match thread count to actual workflow requirements
8. **Account for Overhead:** Use `--no-metrics` for pure storage benchmarking
9. **Document Test Conditions:** Record system state, network conditions, concurrent load

---

## Troubleshooting

### Test Fails or Low Success Rate

**Check:**
- Storage has sufficient free space
- No permission issues on test directory
- Filesystem is healthy (run fsck if needed)
- Network storage is accessible and stable

### Unexpectedly Low Performance

**Causes:**
- Other processes using storage
- Network congestion (for network storage)
- Thermal throttling
- Background system tasks (antivirus, backups, etc.)

**Solutions:**
- Close other applications
- Run test multiple times
- Use `--no-metrics` to reduce overhead
- Check system monitoring tools during test

### TUI Not Displaying

**Requirements:**
- Terminal with TTY support (not available through pipes/redirects)
- ANSI color support
- Minimum 80x24 terminal size

**Workaround:**
- Run without `--tui` flag
- Use standard output mode

---

## See Also

- [USAGE.md](USAGE.md) - Quick start guide
- [FEATURES_GUIDE.md](FEATURES_GUIDE.md) - Feature overview
- [FAQ.md](FAQ.md) - Frequently asked questions
- [CHANGELOG.md](CHANGELOG.md) - Version history

---

**For additional help:**
```bash
vframetest --help
```

**Report issues:**
https://github.com/ssotoa70/vframetest/issues
