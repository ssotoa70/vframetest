# Changelog

All notable changes to vframetest are documented in this file.

## [25.14.0] - 2025-11-30

### Phase 4A: TUI Real-Time Dashboard ✅

#### Added
- **Terminal User Interface (TUI)** for real-time test monitoring
  - Real-time progress and performance metrics display
  - Live throughput (MiB/s) and IOPS tracking
  - Latency percentiles (P50, P95, P99) with sparkline visualization
  - I/O mode tracking (Direct vs Buffered I/O)
  - UTF-8 box-drawing characters for professional appearance

- **Interactive Features**
  - Keyboard controls for navigation
  - Real-time trend analysis (improving/stable/degrading)
  - Pause/resume test capability
  - Help overlay with command reference
  - Configuration menu for test parameters

- **Terminal Support**
  - Auto-detection of TUI capability
  - Works over SSH connections
  - Graceful fallback to text-only mode
  - Support for TERM environment variable
  - Dynamic terminal width detection

#### Technical Enhancements
- Modular architecture (TUI core, state management, rendering, input)
- Thread-safe atomic operations for real-time updates
- Circular buffers for efficient memory usage
- Percentile calculation optimized for 100ms render cycles
- Signal handlers for clean shutdown (SIGINT, SIGTERM)

#### Cross-Platform
- ✅ macOS (arm64, x86_64) with full TUI
- ✅ Linux with full TUI
- ✅ Windows with graceful text-only mode
- ✅ SSH-compatible terminal rendering

#### Code Quality
- ✅ No security vulnerabilities found
- ✅ Comprehensive unit tests (percentile, metrics, edge cases)
- ✅ Zero compiler warnings
- ✅ <1% performance overhead
- ✅ Professional memory management

#### Files Added
- src/tui.h/c - Core TUI implementation
- src/tui_state.h/c - Application state machine
- src/tui_render.h/c - Rendering engine
- src/tui_input.h/c - Keyboard input handling
- src/screen.h/c - Terminal abstraction
- src/tty.h/c - TTY control
- tests/test_tui.c - TUI unit tests

#### Version Changes
- Makefile: Version bumped to 25.14.0
- Phase 4A completion marks major feature milestone

---

## [25.13.1] - 2025-11-30

### Bug Release: DPX/EXR Profile Addition & Profile Filtering

#### Added
- **DPX Frame Profiles** - Professional 10/12-bit uncompressed profiles
  - DPX-2K-10bit (2048×1556, 4 bytes/pixel, 8KB header)
  - DPX-FULLHD-10bit (1920×1080, 4 bytes/pixel, 8KB header)
  - DPX-4K-10bit (3840×2160, 4 bytes/pixel, 8KB header)
  - DPX-8K-10bit (7680×4320, 4 bytes/pixel, 8KB header)

- **EXR Frame Profiles** - Professional float/half-precision profiles
  - EXR-FULLHD-half, EXR-4K-half, EXR-8K-half (6 bytes/pixel, no header)
  - EXR-FULLHD-float, EXR-4K-float, EXR-8K-float (12 bytes/pixel, no header)

- **Profile Filtering** - `--list-profiles-filter` flag for filtering by name prefix
  - Usage: `vframetest --list-profiles-filter DPX` - show only DPX profiles
  - Works standalone or with `--list-profiles`

#### Enhanced
- **--list-profiles Output** - Now displays:
  - Raw size (width × height × bpp + header)
  - Aligned size (4096-byte aligned for Direct I/O)
  - Useful for capacity planning and performance analysis

#### Changed
- Version bumped to 25.13.1
- opts_t structure: Added list_profiles_filter field
- list_profiles() function signature: Now accepts optional filter parameter

#### Test Results
- ✅ All 100+ unit tests passing
- ✅ New DPX/EXR profile assertions passing
- ✅ Zero compiler warnings
- ✅ Cross-platform compatible (macOS, Linux, Windows)

#### Files Modified
- src/profile.h - Added PROF_DPX and PROF_EXR enum types
- src/profile.c - Added 10 new profile definitions
- src/frametest.h - Added filter support to options
- src/frametest.c - Enhanced list_profiles with filtering and sizes
- tests/test_profile.c - New profile validation tests
- Makefile - Version updated to 25.13.1

---

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
