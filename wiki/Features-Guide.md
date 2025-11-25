# vframetest v25.11.23 - Complete Features Guide

**Current Version**: 25.11.23
**Phase**: 1 Complete, 2 In Progress
**Last Updated**: November 25, 2025

---

## Table of Contents

1. [Core Features](#core-features)
2. [Error Tracking & Diagnostics](#error-tracking--diagnostics)
3. [Filesystem Detection](#filesystem-detection)
4. [Performance Metrics](#performance-metrics)
5. [Output Formats](#output-formats)
6. [Platform Support](#platform-support)
7. [Advanced Features](#advanced-features)
8. [Comparison Table](#comparison-table)

---

## Core Features

### Frame-Based Testing

**Overview**: vframetest simulates video frame I/O operations to measure storage performance.

**What It Does**:
- Writes frames of configurable sizes to storage
- Reads frames back to verify integrity
- Tracks timing and throughput metrics
- Supports multi-threaded parallel testing

**Why It Matters**:
- Video workloads are among the most demanding for storage systems
- Frame-based testing better reflects real video production workflows
- More accurate than traditional block-based benchmarks for video storage

**Example**:
```bash
# Test with FULLHD (1920x1080) 24-bit frames, 10 frames on 4 threads
vframetest -w FULLHD-24bit -t 4 -n 10 /path/to/storage

# Output:
Profile: FULLHD-24bit
Results write:
 frames: 10
 bytes : 62873600
 time  : 39645000 ns
 fps   : 252.238618
 B/s   : 1585914995.585824
 MiB/s : 1512.446399
```

### Multi-Threaded Testing

**Capability**: Test storage with configurable thread counts to simulate parallel video processing.

**Usage**:
```bash
# Single-thread (baseline)
vframetest -w FULLHD-24bit -t 1 -n 100 /mnt/storage

# Multi-thread (scalability testing)
vframetest -w FULLHD-24bit -t 8 -n 100 /mnt/storage
vframetest -w FULLHD-24bit -t 16 -n 100 /mnt/storage
```

**What You'll Learn**:
- If storage scales linearly with thread count
- Saturation point of the storage system
- Optimal thread count for your workload

---

## Error Tracking & Diagnostics

### ✨ NEW in v25.11.23: Comprehensive Error Tracking

**What It Captures**:
- System errno values for each I/O error
- Which specific frame failed
- Which thread encountered the error
- Operation type (open, read, write, close)
- Precise timestamp of failure

**Example Output** (when errors occur):
```
Frames failed: 2
Frames succeeded: 98
Success rate: 98.00%
```

**Detailed Error Information**:
Each error includes:
```
Frame: 45
Thread: 2
Operation: write
Error: No space left on device (errno: 28)
Timestamp: 2025-11-25 10:30:45.123456789
```

### Success Rate Metrics

**What It Shows**:
- Total frames that completed successfully
- Total frames that failed
- Success rate percentage
- Per-frame resolution (not batch-level)

**Why It Matters**:
- Identifies intermittent I/O failures
- Detects degraded storage performance
- Helps pinpoint reliability issues

**Example Interpretation**:
```
Frames failed: 0
Frames succeeded: 100
Success rate: 100.00%
→ Perfect reliability - no errors during test
```

---

## Filesystem Detection

### ✨ NEW in v25.11.23: Automatic Filesystem Type Detection

**Supported Filesystem Types**:
1. **LOCAL** - Local storage (SSD, HDD)
2. **SMB** - Samba/CIFS network shares (Windows File Sharing, NAS)
3. **NFS** - Network File System (Linux/Unix network storage)
4. **OTHER** - Unsupported or unknown filesystem types

**How It Works**:
```bash
# macOS example
vframetest -w HD-24bit -t 2 -n 10 /Volumes/LocalSSD
→ Filesystem: LOCAL

vframetest -w HD-24bit -t 2 -n 10 /Volumes/NAS-SMB
→ Filesystem: SMB
→ WARNING: Test path is on a remote filesystem
→ Direct I/O may not be available. Results may not be accurate.
```

### Remote Filesystem Warnings

**What They Indicate**:
- Test is running on network-attached storage
- Direct I/O optimizations may not be available
- Results should be interpreted with caution
- Performance may be affected by network conditions

**Important**: Network filesystems have different characteristics than local storage. Results on SMB/NFS may not be comparable to local tests.

---

## Performance Metrics

### Throughput Metrics

**Measurements Provided**:
- **FPS** (Frames Per Second) - How many frames processed per second
- **B/s** (Bytes Per Second) - Raw throughput in bytes
- **MiB/s** (Mebibytes Per Second) - Practical throughput metric

**Understanding the Numbers**:
```
Profile: FULLHD-24bit
fps   : 172.057811        (172 frames/sec)
B/s   : 1,081,789,401     (~1 billion bytes/sec)
MiB/s : 1031.674768       (~1031 MB/sec) ← Most practical
```

### Completion Time Analysis

**What It Measures**:
- **min**: Fastest frame completion time
- **avg**: Average frame completion time
- **max**: Slowest frame completion time

**Interpretation**:
```
Completion times:
 min   : 3.722000 ms  (best case)
 avg   : 5.802200 ms  (typical)
 max   : 9.801000 ms  (worst case)

Range: 9.8 - 3.7 = 6.1 ms variance
→ Good consistency - predictable performance
```

### Advanced Timing Breakdown (--times flag)

**Available with**: `vframetest --times`

Breaks down completion time by operation:
- **Open times** - File open operation duration
- **I/O times** - Read/write operation duration
- **Close times** - File close operation duration

**Use Case**: Identifying bottleneck operations.

---

## Output Formats

### Standard Text Output

**Default format** - Human readable:
```
Profile: FULLHD-24bit
Results write:
 frames: 5
 bytes : 31436800
 time  : 29060000
 fps   : 172.057811
 B/s   : 1081789401.238816
 MiB/s : 1031.674768
Completion times:
 min   : 3.722000 ms
 avg   : 5.802200 ms
 max   : 9.801000 ms
Frames failed: 0
Frames succeeded: 5
Success rate: 100.00%
Filesystem: LOCAL
```

**Usage**: `vframetest [options] /path`

### CSV Output

**Format**: Comma-separated values for spreadsheet import

```bash
vframetest -c -w FULLHD-24bit -t 1 -n 100 /mnt/storage > results.csv
```

**Output Structure**:
```
case,profile,threads,frames,bytes,time,fps,bps,mibps,fmin,favg,fmax
write,FULLHD-24bit,1,100,625436800,2900600000,344.5,215000000,2048.5,2.1,5.8,12.3
```

**Benefits**:
- Import into Excel/Google Sheets
- Easy comparison of multiple runs
- Suitable for presentations and reports

### JSON Output

**Format**: Machine-readable for programmatic access

```bash
vframetest -j -w FULLHD-24bit -t 1 -n 100 /mnt/storage > results.json
```

**Output Example**:
```json
{
  "case": "write",
  "profile": "FULLHD-24bit",
  "threads": 1,
  "frames": 100,
  "bytes": 625436800,
  "throughput_mibps": 2048.5,
  "fps": 344.5,
  "completion_times": {
    "min_ms": 2.1,
    "avg_ms": 5.8,
    "max_ms": 12.3
  }
}
```

**Benefits**:
- Parse in Python/Node.js scripts
- Store in databases
- Feed to dashboards and analytics tools
- Automated performance tracking

### Histogram Output

**Visualization** of latency distribution:

```bash
vframetest --histogram -w FULLHD-24bit -t 2 -n 100 /mnt/storage
```

**Shows**:
- Distribution of frame completion times
- Identifies outliers and jitter
- Visual representation of performance consistency

---

## Platform Support

### Supported Operating Systems

#### macOS
- **Versions**: 10.13+
- **Architectures**: Apple Silicon (arm64), Intel (x86_64)
- **Installation**: `brew install ssotoa70/vframetest/vframetest`
- **Binary**: Universal/arm64/x86_64 variants

#### Linux
- **Distributions**: Ubuntu 18.04+, Debian 10+, CentOS 7+, Fedora 30+
- **Architecture**: x86_64
- **Installation**: Download binary or build from source
- **Supported filesystems**: ext4, btrfs, XFS, NFS, SMB

#### Windows
- **Versions**: Windows 10 2004+, Windows 11
- **Architectures**: x86_64, i686
- **Installation**: Download executable
- **Binary**: `.exe` files for both architectures

### Filesystem Support

**Tested On**:
- Local storage: SSD (NVMe, SATA), HDD (mechanical)
- Network storage: SMB shares, NFS mounts, iSCSI
- Cloud storage: AWS EBS, Azure managed disks, Google Persistent Disks
- Virtual storage: VMFS, Hyper-V, KVM volumes

---

## Advanced Features

### Custom Frame Profiles

**NEW in v25.11.22**: Define your own frame profiles

**Predefined Profiles Available**:
```
SD-32bit-cmp    - 720x480, 32-bit
HD-24bit        - 1280x720, 24-bit
FULLHD-24bit    - 1920x1080, 24-bit (Popular choice)
4K-24bit        - 3840x2160, 24-bit
8K-24bit        - 7680x4320, 24-bit
empty           - Custom size frames
```

**Using Custom Sizes**:
```bash
# Test with custom 2560x1440 resolution
# Set up custom profile or use -z flag
vframetest -e -z 2560x1440 -t 2 -n 50 /mnt/storage
```

### Streaming Mode

**Purpose**: Test single file with sequential frame writes.

```bash
vframetest -s output.raw -z 1920x1080x3 -n 100 /mnt/storage
```

**Use Case**:
- Single-file streaming performance
- Video capture/playback simulation
- Different I/O pattern than multi-file

### Access Patterns

#### Sequential Access (Default)
```bash
vframetest -w FULLHD-24bit -n 100 /mnt/storage
```

#### Reverse Access
```bash
vframetest -v -w FULLHD-24bit -n 100 /mnt/storage
```
Reads/writes files in reverse order.

#### Random Access
```bash
vframetest -m -w FULLHD-24bit -n 100 /mnt/storage
```
Randomizes file access order.

**Why It Matters**:
- Sequential: Natural for video playback
- Reverse: Edge case testing
- Random: Tests cache behavior and seek times

### Frame Timing Data

**Export detailed per-frame timing**:
```bash
vframetest --frametimes -c -w FULLHD-24bit -n 10 /mnt/storage
```

**Output**:
```csv
frame,start,open,io,close,frame
0,1000,100,2000,500,2600
1,3600,95,2100,510,2705
...
```

**Use**: Identify performance anomalies in specific frames.

---

## Comparison Table

### Features by Version

| Feature | v25.11.21 | v25.11.22 | v25.11.23 |
|---------|-----------|-----------|-----------|
| Frame testing | ✅ | ✅ | ✅ |
| CSV output | ✅ | ✅ | ✅ |
| JSON output | ✅ | ✅ | ✅ |
| Multi-threaded | ✅ | ✅ | ✅ |
| Custom profiles | ❌ | ✅ | ✅ |
| Streaming mode | ✅ | ✅ | ✅ |
| Histogram | ✅ | ✅ | ✅ |
| Benchmarking | ❌ | ✅ | ✅ |
| **Error tracking** | ❌ | ❌ | ✅ |
| **Filesystem detection** | ❌ | ❌ | ✅ |
| **Success metrics** | ❌ | ❌ | ✅ |
| **Remote FS warnings** | ❌ | ❌ | ✅ |
| Windows support | ✅ | ✅ | ✅ |
| macOS support | ✅ | ✅ | ✅ |
| Linux support | ✅ | ✅ | ✅ |

---

## Common Use Cases

### Storage Performance Validation
```bash
# Comprehensive test: 1K frames on 4 threads, all metrics
vframetest -w 4K-24bit -t 4 -n 1000 --times --histogram -c /mnt/storage
```

### Video Workload Simulation
```bash
# HD video: 24 fps * 60 seconds = 1440 frames
vframetest -w HD-24bit -t 2 -n 1440 /media/storage
```

### Network Storage Validation
```bash
# Test SMB share with error tracking
vframetest -w FULLHD-24bit -t 2 -n 100 //nas-server/share
# Will warn if SMB detected
```

### Automated Benchmarking
```bash
# Daily performance tracking
for dir in /mnt/storage-*; do
  vframetest -j -w FULLHD-24bit -t 4 -n 500 "$dir" > "results-$(date +%s).json"
done
```

### Performance Regression Detection
```bash
# Compare results between builds
vframetest -c -w FULLHD-24bit -t 2 -n 1000 /mnt/storage | tee today.csv
# Compare with yesterday.csv using diff or graphical tools
```

---

## Performance Tips

### For Accurate Results
1. **Warm up the cache**: Run test twice, use second run
2. **Isolate the system**: Close other applications
3. **Use appropriate frame count**: Minimum 100 frames for meaningful data
4. **Test multiple times**: Run tests 3+ times, average results

### For Maximum Throughput
1. **Increase threads**: 4-8 threads typical for modern systems
2. **Increase frame count**: Longer tests amortize overhead
3. **Use sequential access**: Default is fastest for most systems

### For Remote Filesystem Testing
1. **Increase timeout**: Network latency adds variability
2. **Test multiple times**: Network conditions may vary
3. **Check warnings**: Heed remote filesystem warnings
4. **Consider direct I/O**: Ask admin about direct I/O capability

---

## Troubleshooting

### "Permission Denied" Error
```
Solution: Ensure write access to target directory
chmod +w /mnt/storage
```

### Low Performance Results
```
Check:
- System is busy (other applications running)
- Filesystem is remote (use warning)
- Storage is saturated (run again later)
- Direct I/O not available on remote FS
```

### High Frame Failure Rate
```
Check:
- Disk space available (Frames failed error)
- Filesystem health
- I/O timeout on remote filesystems
```

---

## What's Coming in Phase 2

- Graceful I/O fallback (direct I/O → buffered)
- CSV export of error details
- Detailed error breakdown reports

---

**For Latest Updates**: See https://github.com/ssotoa70/vframetest/releases
