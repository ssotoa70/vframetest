# vframetest - Professional Frame-Based Storage I/O Benchmark

[![CI/CD](https://github.com/ssotoa70/vframetest/actions/workflows/ci.yml/badge.svg)](https://github.com/ssotoa70/vframetest/actions/workflows/ci.yml)
[![GitHub Release](https://img.shields.io/github/v/release/ssotoa70/vframetest)](https://github.com/ssotoa70/vframetest/releases)
[![License](https://img.shields.io/badge/license-GPL%202.0-blue)](COPYING)

**Enterprise-grade storage performance validation tool** designed to accurately measure I/O performance for video workloads, media production, and data center infrastructure.

Evolved from the original Tuxera/SGI implementation into a modern, multi-platform solution with comprehensive error tracking, filesystem detection, and professional CI/CD infrastructure.

---

## 🎯 Why vframetest?

- **Video-Centric Benchmarking**: Tests actual video frame I/O patterns, not generic block I/O
- **Cross-Platform**: Single tool for macOS, Linux, and Windows
- **Production-Ready**: Error tracking, filesystem detection, comprehensive diagnostics
- **Easy to Use**: Pre-built binaries, Homebrew installation, simple CLI
- **Well-Documented**: 2000+ lines of comprehensive documentation
- **Community-Focused**: Open governance, clear contribution path, professional standards

---

## 🚀 Quick Start

### Installation

**macOS (Homebrew)**
```bash
brew tap ssotoa70/vframetest https://github.com/ssotoa70/vframetest
brew install vframetest
vframetest --version
```

**Linux & Windows** - [Download pre-built binaries](https://github.com/ssotoa70/vframetest/releases)

### Basic Test
```bash
# Create test directory
mkdir -p test-data

# Run 100 FULLHD frames on 4 threads
vframetest -w FULLHD-24bit -t 4 -n 100 test-data

# Expected output:
# Frames failed: 0
# Frames succeeded: 100
# Success rate: 100.00%
# Filesystem: LOCAL
# [Performance metrics...]
```

---

## 📊 Key Features

### Core Benchmarking
- ✅ **Frame-Based Testing** - Accurate video workload simulation
- ✅ **Multi-Threading** - Configurable parallel testing (1-16+ threads)
- ✅ **Multiple Profiles** - SD, HD, 4K, 8K + custom resolutions
- ✅ **Rich Metrics** - FPS, throughput (MiB/s), completion times

### Phase 1: Error Tracking & Diagnostics
- ✅ **Comprehensive Error Capture** - errno tracking, frame-level detail
- ✅ **Success Rate Metrics** - Per-frame success/failure counting
- ✅ **Filesystem Detection** - LOCAL, SMB, NFS, OTHER types
- ✅ **Remote FS Warnings** - Alerts for network storage limitations
- ✅ **Direct I/O Checking** - Availability detection per platform

### Phase 2: I/O Fallback & Enhanced Reporting
- ✅ **Graceful I/O Fallback** - Automatic Direct I/O → Buffered I/O fallback
- ✅ **Per-Frame I/O Tracking** - Track which frames used Direct vs Buffered I/O
- ✅ **Enhanced Error Reporting** - Error statistics and per-operation breakdown
- ✅ **CSV/JSON Error Export** - Detailed error data with timestamps
- ✅ **Fallback Statistics** - Direct I/O success rate and fallback metrics

### Phase 3: NFS/SMB Optimization Detection
- ✅ **Automatic NFS/SMB Optimization** - Skip Direct I/O on remote filesystems
- ✅ **Performance Trend Analysis** - Track improving/stable/degrading performance
- ✅ **Network Timeout Handling** - Configurable timeouts for network filesystems
- ✅ **Performance Metrics** - Min/max/avg frame times with trend analysis
- ✅ **Enhanced Output** - CSV and JSON include optimization metrics

### Professional Infrastructure
- ✅ **Multi-Platform** - macOS (arm64/x86_64), Linux, Windows (x86_64/i686)
- ✅ **CI/CD Automation** - GitHub Actions with multi-platform builds
- ✅ **Package Management** - Homebrew on macOS, pre-built binaries elsewhere
- ✅ **Output Formats** - Text, CSV, JSON, histograms
- ✅ **Extensive Documentation** - 2000+ lines of guides and references

---

## 📈 Supported Configurations

| Aspect | Details |
|--------|---------|
| **Platforms** | macOS 10.13+, Linux (glibc), Windows 10+ |
| **Architectures** | arm64, x86_64, i686, universal (macOS) |
| **Frame Sizes** | SD (720×480), HD (1280×720), FULLHD (1920×1080), 4K, 8K, custom |
| **Threading** | 1 to 16+ concurrent threads |
| **Filesystems** | LOCAL, SMB/CIFS, NFS, others (with detection) |
| **Output Formats** | Text, CSV, JSON, histograms, per-frame timing |

---

## 📚 Documentation

**Start Here**:
- [Quick Start](#quick-start) - Get up and running in 2 minutes
- [Installation Guide](docs/INSTALLATION.md) - All installation methods
- [Usage Guide](docs/USAGE.md) - Complete feature documentation

**Learn More**:
- [Architecture & Design](docs/ARCHITECTURE.md) - Technical deep dive
- [Project Evolution](docs/PROJECT_EVOLUTION.md) - History from original to modern
- [Feature Guide](docs/FEATURES_GUIDE.md) - Complete feature reference (600+ lines)
- [FAQ](docs/FAQ.md) - Common questions and troubleshooting

**Project Info**:
- [Contributing](docs/CONTRIBUTING.md) - How to contribute
- [Code of Conduct](docs/CODE_OF_CONDUCT.md) - Community guidelines
- [Governance](docs/GOVERNANCE.md) - Decision-making framework
- [Roadmap](docs/ROADMAP.md) - Future features and vision
- [Security](docs/SECURITY.md) - Security considerations

**Reference**:
- [Repository Structure](docs/REPOSITORY_STRUCTURE.md) - File organization
- [Changelog](docs/CHANGELOG.md) - Version history
- [Phase 1 Completion](docs/PHASE_1_COMPLETION_REPORT.md) - Technical report
- [Phase 3 Completion](docs/PHASE_3_COMPLETION_REPORT.md) - NFS/SMB Optimization report

---

## 💡 Common Use Cases

### Storage Validation
```bash
# Comprehensive test: 1000 FULLHD frames, 4 threads, all metrics
vframetest -w FULLHD-24bit -t 4 -n 1000 /mnt/storage
```

### Performance Regression Detection
```bash
# Save baseline
vframetest -c -w FULLHD-24bit -t 2 -n 500 /mnt/storage > baseline.csv

# Compare against current (use diff or graphical tools)
vframetest -c -w FULLHD-24bit -t 2 -n 500 /mnt/storage > current.csv
```

### Network Storage Analysis
```bash
# Test SMB share (vframetest auto-detects and warns about remote FS)
vframetest -w FULLHD-24bit -t 2 -n 100 //nas-server/share
# Output will include:
# WARNING: Test path is on a remote filesystem
# Direct I/O may not be available. Results may not be accurate.
```

### JSON Export for Automation
```bash
# Export results for dashboards and analysis
vframetest -j -w FULLHD-24bit -t 4 -n 100 /mnt/storage > results.json

# Parse and process with Python/Node.js for custom analytics
```

---

## 🔧 Building from Source

### Requirements
- C99 compiler (gcc, clang)
- POSIX threads (pthread)
- Make (GNU make preferred)
- Git (for version tracking)

### Quick Build
```bash
git clone https://github.com/ssotoa70/vframetest.git
cd vframetest
make clean && make -j4
./build/vframetest --version
```

### Platform-Specific
```bash
# macOS
make clean && make -j4

# Linux
make clean && make -j4

# Windows (requires MinGW)
make win64        # 64-bit
make win          # 32-bit
```

For detailed build instructions, see [BUILD](docs/BUILD.md).

---

## 📊 Example Output

```
Profile: FULLHD-24bit
Results write:
 frames: 100
 bytes : 6254368000
 time  : 2900600000 ns
 fps   : 344.5
 B/s   : 2155000000
 MiB/s : 2055.34
Completion times:
 min   : 2.1 ms
 avg   : 5.8 ms
 max   : 12.3 ms
Frames failed: 0
Frames succeeded: 100
Success rate: 100.00%
Filesystem: LOCAL
```

---

## 🤝 Contributing

We welcome contributions! See [CONTRIBUTING.md](docs/CONTRIBUTING.md) for:
- Development setup
- Code standards
- Pull request process
- Reporting issues
- Suggesting features

**Community Standards**: All contributors must follow our [Code of Conduct](docs/CODE_OF_CONDUCT.md)

---

## 📈 Project Status

| Phase | Status | Features |
|-------|--------|----------|
| **Phase 1** | ✅ Complete | Error tracking, filesystem detection, success metrics |
| **Phase 2** | ✅ Complete | Graceful I/O fallback, error export, detailed reports |
| **Phase 3** | ✅ Complete | NFS/SMB optimization detection, performance analysis |
| **Phase 4** | 🚀 In Development | Web dashboard, real-time metrics, REST API |
| **Future** | 🔮 Vision | Cloud integration, distributed testing, ML anomaly detection |

Current Version: **25.13.0** (Phase 3 Complete - NFS/SMB Optimization)
Latest Release: [v25.13.0](https://github.com/ssotoa70/vframetest/releases/tag/v25.13.0)

---

## 📞 Support & Community

- **Issues & Bugs**: [GitHub Issues](https://github.com/ssotoa70/vframetest/issues)
- **Discussions**: [GitHub Discussions](https://github.com/ssotoa70/vframetest/discussions)
- **Documentation**: [Complete Guides](docs/)
- **Wiki**: [Community Wiki](/wiki)

---

## 📜 License

GNU General Public License v2 - See [COPYING](COPYING) for details.

**Origin**: Extended from original Tuxera Inc. / SGI implementation
**Modern Fork**: Enhanced with multi-platform support, comprehensive error tracking, and professional infrastructure

---

## 🙏 Acknowledgments

- **Original**: Tuxera Inc. / SGI for foundational frame-based testing approach
- **Contributors**: Community members who have contributed fixes and features
- **Users**: Thank you for using and improving vframetest

---

**Repository**: [ssotoa70/vframetest](https://github.com/ssotoa70/vframetest)
**Latest Release**: [v25.11.23](https://github.com/ssotoa70/vframetest/releases/tag/v25.11.23)
**Documentation**: [Complete Guides](docs/) | [Wiki](/wiki) | [FAQ](docs/FAQ.md)
