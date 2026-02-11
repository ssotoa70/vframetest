# Frequently Asked Questions

## General

### Q: What is vframetest?
A: vframetest is a frame-based I/O benchmark tool that simulates video workloads to measure storage performance. It's particularly useful for validating storage systems for video production, media workflows, and data centers.

### Q: Why not use generic I/O benchmarks?
A: Video storage has unique characteristics - sequential frame writes/reads at high throughput. vframetest simulates actual video I/O patterns rather than generic block I/O, providing more accurate performance assessment.

### Q: How is vframetest different from the original Tuxera version?
A: vframetest extends the original with:
- Multi-platform support (macOS, Linux, Windows)
- Comprehensive error tracking
- Filesystem detection with warnings
- Professional CI/CD infrastructure
- Extensive documentation

## Installation

### Q: Can I use vframetest on Windows?
A: Yes! Pre-built binaries are available for Windows x86_64 and i686.

### Q: How do I install on macOS without Homebrew?
A: Download pre-built binary from [releases page](https://github.com/ssotoa70/vframetest/releases) and place in your PATH.

### Q: Do I need administrator privileges?
A: Generally no. You only need read/write permissions to the test directory.

## Usage

### Q: How many frames should I test?
A: Minimum 100 frames for meaningful results. 1000+ frames recommended for comprehensive testing.

### Q: What's a good thread count?
A: Start with 2-4 threads, then test with higher counts (8, 16) to find your storage saturation point.

### Q: Should I test multiple times?
A: Yes! Run tests 3+ times and average results to account for OS caching and system variability.

### Q: What do "Frames failed" mean?
A: I/O errors during frame operations. 0 is expected under normal conditions. Non-zero indicates I/O problems.

### Q: Why is my remote filesystem slower?
A: Network storage has inherent latency and is bound by network bandwidth. vframetest warns about remote filesystems for this reason.

## Latency Metrics & Accuracy

### Q: Should I upgrade to v25.17.4?
A: **YES - IMMEDIATELY** if you're using v25.17.3 or earlier. v25.17.4 fixes a critical bug where latency values were reported as 2.6×10^15 times larger than actual. Example: A 6.97ms latency was reported as 18+ quadrillion milliseconds in v25.17.3.

### Q: How do I know if I need to upgrade?
A: Upgrade to v25.17.4 if:
- You're using vframetest v25.17.3 or earlier
- You use completion time metrics (min/avg/max latency)
- You benchmark storage performance and track completion times

### Q: What about my old benchmark results?
A: Benchmark results using v25.17.3 or earlier that relied on completion time metrics are inaccurate. After upgrading to v25.17.4:
1. Discard old completion time results
2. Re-run tests with v25.17.4
3. Use v25.17.4 results as your new baseline
4. Compare all future tests against the v25.17.4 baseline

### Q: Will v25.17.4 break my workflow?
A: No. The upgrade is transparent - same command-line arguments, no configuration changes needed. Simply upgrade and continue as before.

## Performance

### Q: Why are my results inconsistent?
A: Possible causes:
- System caching (run twice, use second result)
- Background processes (close other apps)
- Network variability (for remote storage)
- Thermal throttling (let system cool between tests)

### Q: What's a typical throughput?
A: Depends on your storage:
- NVMe SSD: 1000-3000 MiB/s
- SATA SSD: 400-600 MiB/s
- HDD: 100-200 MiB/s
- Network: varies (10-500 MiB/s typical)

### Q: What if direct I/O is not available?
A: vframetest will use buffered I/O. Results will be less accurate (affected by OS cache). Phase 2 will handle this better.

## Output

### Q: What's the difference between CSV and JSON output?
A: CSV is better for spreadsheets; JSON is better for programmatic processing. Choose based on your workflow.

### Q: Can I export individual frame timings?
A: Use `--frametimes` flag to export per-frame timing data in CSV format.

### Q: How do I interpret the histogram?
A: The histogram shows distribution of frame completion times. Taller left side = consistent performance. Spread toward right = variable performance.

## Troubleshooting

### Q: "Permission denied" error
A: Check you have write permissions to test directory:
```bash
ls -ld /test/directory
# Should show write permission (w) for your user
```

### Q: "No space left on device"
A: Ensure adequate free space. FULLHD-24bit × 100 frames needs ~630 MB.

### Q: Build fails with compiler errors
A: Ensure you have C99-compatible compiler. Try:
```bash
gcc --version    # Check GCC version
clang --version  # Or Clang
make clean && make -j4
```

### Q: Binary works but seems slow
A: Possible causes:
- Test on local SSD (not network storage)
- Close background applications
- Check filesystem (local vs SMB/NFS)
- Review system resource usage

## Contributing

### Q: How can I contribute?
A: See [CONTRIBUTING.md](docs/CONTRIBUTING.md) for:
- Development setup
- Code standards
- Pull request process

### Q: How do I report bugs?
A: Create issue on [GitHub Issues](https://github.com/ssotoa70/vframetest/issues) with:
- Detailed description
- Steps to reproduce
- Your environment (OS, hardware)
- Actual vs expected behavior

### Q: Where can I request features?
A: Create GitHub issue with "enhancement" label describing:
- The feature and use case
- Why it would be valuable
- Suggested implementation approach

## Licensing

### Q: Is vframetest free?
A: Yes, vframetest is open source under GPL v2.

### Q: Can I use vframetest commercially?
A: Yes, with GPL v2 compliance. See [COPYING](../COPYING) file for details.

### Q: Can I modify vframetest?
A: Yes, under GPL v2 terms. If you distribute modified versions, source must be available.

---

**Still have questions?** 
- Check [documentation](../docs/)
- Search [GitHub Issues](https://github.com/ssotoa70/vframetest/issues)
- Contact ssotoa70@gmail.com
