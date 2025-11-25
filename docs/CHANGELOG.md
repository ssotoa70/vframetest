# Changelog

All notable changes to vframetest are documented in this file.

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
