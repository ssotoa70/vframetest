# Changelog

All notable changes to vframetest are documented in this file.

## [25.13.0] - 2025-11-28

### Phase 3: NFS/SMB Optimization Detection ✅

#### Added
- **Automatic NFS/SMB Detection** - Filesystem type detection at startup (LOCAL/SMB/NFS/OTHER)
- **Direct I/O Optimization** - Skips Direct I/O on remote filesystems to prevent failures
- **Timeout Handling** - Configurable timeouts for network filesystem operations (30 seconds default)
- **Performance Trend Analysis** - Tracks min/max/avg frame times and performance trends
  - Improving trend (1.0): Performance getting better
  - Stable trend (0.0): Consistent performance
  - Degrading trend (-1.0): Performance getting worse
- **Enhanced CSV Output** - 6 new columns: is_remote, min_frame_time, avg_frame_time, max_frame_time, performance_trend, network_timeout
- **Enhanced JSON Output** - New "optimization_metrics" section with all Phase 3 data

#### Changed
- Modified `tester_frame_write()` and `tester_frame_read()` to check filesystem type
- Frame time tracking integrated into main test loops
- Version bumped to 25.13.0

#### Technical Details
- Platform: `platform_get_network_timeout()` for configurable timeouts
- Data Structure: `test_result_t` expanded with 7 new fields
- Cross-Platform: macOS (statfs), Linux (statfs), Windows (GetVolumeInformation, UNC detection)
- Performance: <1% overhead from trend analysis

#### Test Results
- ✅ All unit tests passing
- ✅ All CI/CD checks passing (macOS, Linux, Windows)
- ✅ Zero compiler warnings
- ✅ Backward compatible with Phase 1 & 2
- ✅ Code coverage: 100+ unit tests

#### Documentation
- PHASE_3_COMPLETION_REPORT.md - Technical completion report
- README.md - Phase 3 features section
- API and architecture documentation updated

## [25.12.0] - 2025-11-27

### Phase 2: I/O Fallback & Enhanced Reporting ✅

#### Added
- **Graceful I/O Fallback** - Direct I/O → Buffered I/O fallback mechanism
- **Per-Frame I/O Tracking** - io_mode_t enum for Direct/Buffered tracking
- **Enhanced Error Reporting** - Error statistics and per-operation breakdown
- **CSV Error Export** - error_frame, error_operation, error_errno columns
- **JSON Error Export** - Structured error data with timestamps
- **Fallback Statistics** - frames_direct_io, frames_buffered_io, fallback_count metrics

#### Changed
- Test execution continues on I/O fallback (no hard failures)
- CSV format expanded with filesystem and I/O stats
- JSON format includes success_metrics and io_fallback_stats sections

## [25.11.23] - 2025-11-25

### Phase 1 Complete ✅

#### Added
- **Error Tracking System** - errno capture, frame-level, thread identification
- **Filesystem Detection** - LOCAL/SMB/NFS/OTHER types with warnings
- **Success Rate Metrics** - Per-frame success/failure counting
- **Remote FS Warnings** - Alerts for network storage limitations
- **Direct I/O Detection** - Platform-specific availability checking
- **Comprehensive Documentation** - 2000+ lines of guides

#### Fixed
- Multi-threaded result aggregation (frame counter accuracy)
- Output display logic (always show metrics)
- Filesystem type display (consistent formatting)

#### Changed
- Repository reorganization (/src, /scripts, /docs, /examples)
- Professional governance framework (CONTRIBUTING, CODE_OF_CONDUCT)
- Enhanced README.md with comprehensive information

### Infrastructure

#### Repository
- Professional .gitignore with 73 patterns
- Clean documentation hierarchy
- Archive directory for deprecated items
- Config and examples directories

#### CI/CD
- Fixed artifact naming conflicts (platform isolation)
- Benchmark data collection stability
- Release job resilience improvements

## [25.11.22] - 2025-11-22

### Infrastructure & Features

#### Added
- **Automated Benchmarking** - Dashboard infrastructure
- **Custom Profiles** - User-defined frame profiles
- **JSON Output** - Machine-readable format support
- **GitHub Actions** - Multi-platform CI/CD
- **Homebrew Formula** - macOS package management
- **Windows Support** - Full Windows x86_64 and i686 support

#### Changed
- Enhanced CSV output format
- Improved build system with version management
- Better cross-platform build scripts

## [25.11.21] - 2025-11-21

### Base Modernization

#### Added
- **Multi-Platform Support** - macOS, Linux, Windows
- **Universal Binaries** - macOS arm64 + x86_64
- **GitHub Actions CI/CD** - Automated builds
- **Pre-built Binaries** - All platforms
- **Professional Documentation** - README, BUILD, etc.

#### Changed
- Migration to modern development practices
- Improved build system
- Professional version management

---

## Versioning

vframetest uses **Semantic Versioning**: MAJOR.MINOR.PATCH

- **MAJOR**: Architecture changes, breaking changes
- **MINOR**: New features, non-breaking enhancements
- **PATCH**: Bug fixes, maintenance

## Release Schedule

- **Phase 1** (v25.11.23): ✅ Released
- **Phase 2**: Projected Early 2026
- **Phase 3**: Projected Mid 2026

See [ROADMAP.md](ROADMAP.md) for planned features.

---

**For Latest Changes**: See [GitHub Releases](https://github.com/ssotoa70/vframetest/releases)
