# Usage Guide

## Basic Syntax

```bash
vframetest [OPTIONS] <test-directory>
```

## Common Options

| Option | Description | Example |
|--------|-------------|---------|
| `-w PROFILE` | Frame profile (SD, HD, FULLHD, 4K, 8K) | `-w FULLHD-24bit` |
| `-t COUNT` | Number of threads (1-16+) | `-t 4` |
| `-n FRAMES` | Number of frames to test | `-n 100` |
| `-c` | CSV output format | `-c` |
| `-j` | JSON output format | `-j` |
| `--times` | Include detailed timing breakdown | `--times` |
| `--histogram` | Show latency histogram | `--histogram` |
| `-s FILE` | Streaming mode (single file) | `-s output.raw` |
| `-v` | Reverse access order | `-v` |
| `-m` | Random access order | `-m` |

## Examples

### Basic Test
```bash
# Single-threaded test with 100 FULLHD frames
vframetest -w FULLHD-24bit -t 1 -n 100 /mnt/storage
```

### Multi-threaded Performance Test
```bash
# 4 threads, 1000 frames, detailed timing
vframetest -w FULLHD-24bit -t 4 -n 1000 --times /mnt/storage
```

### CSV Export for Analysis
```bash
# Export results for spreadsheet import
vframetest -c -w FULLHD-24bit -t 2 -n 500 /mnt/storage > results.csv
```

### JSON Export for Automation
```bash
# Export for programmatic processing
vframetest -j -w FULLHD-24bit -t 4 -n 100 /mnt/storage > results.json
```

### Remote Filesystem Test
```bash
# vframetest auto-detects SMB/NFS and warns about limitations
vframetest -w FULLHD-24bit -t 2 -n 100 //nas-server/share
```

### Custom Frame Size
```bash
# Test with custom resolution (wxhxd where d is bytes per pixel)
vframetest -e -z 2560x1440x3 -t 2 -n 100 /mnt/storage
```

## Output Interpretation

### Success Rate Metrics
```
Frames failed: 0
Frames succeeded: 100
Success rate: 100.00%
```
- Indicates frame-level reliability
- 100% = all frames completed successfully
- < 100% = I/O errors or failures detected

### Performance Metrics
```
fps      : 172.06          (frames per second)
MiB/s    : 1031.67         (throughput - most useful metric)
B/s      : 1,081,789,401   (bytes per second)
```

### Filesystem Detection
```
Filesystem: LOCAL
```
- LOCAL = local storage (most accurate benchmarking)
- SMB = network share (less reliable results)
- NFS = network filesystem (less reliable results)
- Warning shown for remote filesystems

### Completion Times
```
min  : 3.7 ms    (fastest frame)
avg  : 5.8 ms    (average)
max  : 9.8 ms    (slowest frame)
```
- Small range = consistent performance
- Large range = variable performance (network, etc.)

## Workflow Examples

### Storage Validation
```bash
# Comprehensive test
vframetest -w 4K-24bit -t 4 -n 1000 --times --histogram /mnt/storage
```

### Performance Regression Detection
```bash
# Baseline
vframetest -c -w FULLHD-24bit -t 2 -n 500 /mnt/storage > baseline.csv

# Current build
vframetest -c -w FULLHD-24bit -t 2 -n 500 /mnt/storage > current.csv

# Compare (use diff, graphical tools, or scripts)
diff baseline.csv current.csv
```

### Continuous Monitoring
```bash
# Daily benchmarking script
for i in {1..10}; do
  vframetest -j -w FULLHD-24bit -t 4 -n 100 /mnt/storage > results-$(date +%s).json
  sleep 3600  # Wait 1 hour
done
```

## Tips & Best Practices

1. **Warm up the cache** - Run test twice, use second run for results
2. **Use adequate frame count** - Minimum 100 frames for meaningful data
3. **Test multiple times** - Average results over 3+ runs
4. **Isolate the system** - Close other applications during testing
5. **Test at different times** - Capture variability in network/shared storage
6. **Document conditions** - Record system state when testing

---

**Next**: See [FAQ.md](FAQ.md) for common questions and troubleshooting
