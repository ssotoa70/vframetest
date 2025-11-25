# Repository Structure

This document describes the organization and contents of the vframetest repository.

## Directory Layout

```
vframetest/
├── Source Files
│   ├── frametest.c/h      Main driver and test orchestration
│   ├── tester.c/h         Test execution and result aggregation
│   ├── platform.c/h       Platform abstraction (macOS, Linux, Windows)
│   ├── report.c/h         Output formatting (CSV, JSON)
│   ├── histogram.c/h      Performance visualization
│   ├── profile.c/h        Frame profile definitions
│   ├── frame.c/h          Frame data structures
│   └── timing.c/h         High-resolution timing
│
├── Build & Configuration
│   ├── Makefile           Build system with multi-platform support
│   ├── build_win.sh       Windows build script
│   ├── build_linux.sh     Linux build script
│   ├── build_windows.sh   Windows build script (alternative)
│   ├── build_all.sh       Multi-platform build script
│   ├── .clang-format      Code formatting rules
│   └── .github/
│       └── workflows/
│           └── ci.yml     GitHub Actions CI/CD pipeline
│
├── Source Organization
│   ├── build/             Compiled objects and binaries (generated)
│   ├── tests/             Unit and integration tests
│   ├── test-data/         Test data fixtures
│   └── releases/          Historical releases (archive)
│
├── Documentation
│   ├── README.md          Main project overview and quick start
│   ├── BUILD.md           Detailed build instructions
│   ├── CONTRIBUTING.md    Contribution guidelines
│   ├── CODE_OF_CONDUCT.md Community guidelines
│   ├── MAINTAINERS.md     Project maintainers
│   ├── GOVERNANCE.md      Decision-making processes
│   ├── COPYING            GNU GPL v2 License
│   │
│   ├── docs/              Detailed documentation
│   │   ├── PROJECT_EVOLUTION.md    Complete project history
│   │   ├── FEATURES_GUIDE.md       Comprehensive feature documentation
│   │   ├── DEPLOYMENT.md           Production deployment guide
│   │   ├── RUNBOOKS.md             Operational procedures
│   │   ├── MACOS.md                macOS-specific information
│   │   ├── LINUX.md                Linux-specific information
│   │   ├── WINDOWS.md              Windows-specific information
│   │   └── RANCHER-DESKTOP-SETUP.md Development environment setup
│   │
│   ├── wiki/              GitHub-compatible wiki pages
│   │   ├── README.md      Wiki navigation guide
│   │   ├── Home.md        Welcome page
│   │   ├── Project-Evolution.md    Evolution from original to modern
│   │   ├── Features-Guide.md       Feature documentation
│   │   ├── Phase-1-Completion.md   Phase 1 technical details
│   │   └── Comprehensive-Summary.md Master reference document
│   │
│   ├── PHASE_1_COMPLETION_REPORT.md  Technical implementation details
│   ├── RELEASE_SUMMARY.md             Version comparison and timeline
│   ├── COMPREHENSIVE_PROJECT_SUMMARY.md Master project overview
│   └── ANALYSIS_AND_IMPROVEMENTS.md   Technical analysis
│
├── Package Management
│   └── Formula/
│       └── vframetest.rb Homebrew formula for macOS installation
│
├── Infrastructure
│   ├── scripts/
│   │   └── collect-benchmarks.sh Automated benchmarking data collection
│   └── dashboard/
│       ├── data/         Benchmark and performance data (generated)
│       └── README.md     Dashboard documentation
│
└── Repository Configuration
    ├── .git/             Git repository metadata
    ├── .gitignore        Git exclusion rules
    └── .github/
        └── workflows/
            └── ci.yml    GitHub Actions configuration
```

## File Categories

### Core Implementation
- **Source Files** (*.c, *.h): Main application code
- **Build System**: Makefile with cross-platform support
- **Build Scripts**: Platform-specific build automation

### Quality Assurance
- **Tests** (`tests/`): Unit and integration test code
- **Test Data** (`test-data/`): Fixture data for testing
- **Code Formatting** (`.clang-format`): Coding style rules

### Documentation
- **User-Facing**: README.md, docs/, wiki/
- **Developer-Facing**: CONTRIBUTING.md, BUILD.md, GOVERNANCE.md
- **Community**: CODE_OF_CONDUCT.md, MAINTAINERS.md
- **Project History**: PROJECT_EVOLUTION.md, PHASE_1_COMPLETION_REPORT.md

### Infrastructure
- **CI/CD**: .github/workflows/ci.yml for automated testing and releases
- **Package Management**: Formula/vframetest.rb for Homebrew
- **Benchmarking**: scripts/collect-benchmarks.sh and dashboard/ data

## Important Files

### For Users
- **README.md** - Start here for overview and quick start
- **FEATURES_GUIDE.md** - Learn what the tool can do
- **BUILD.md** - How to build from source
- **docs/DEPLOYMENT.md** - Production deployment

### For Contributors
- **CONTRIBUTING.md** - How to contribute to the project
- **CODE_OF_CONDUCT.md** - Community guidelines
- **GOVERNANCE.md** - Decision-making processes

### For Maintainers
- **MAINTAINERS.md** - Maintainer responsibilities
- **GOVERNANCE.md** - Project governance
- **.github/workflows/ci.yml** - CI/CD pipeline

### For Learning
- **PROJECT_EVOLUTION.md** - How this fork evolved from original
- **PHASE_1_COMPLETION_REPORT.md** - Technical implementation details
- **COMPREHENSIVE_PROJECT_SUMMARY.md** - Complete project overview

## Ignored Files (see .gitignore)

The following are **not** tracked in git:
- Build artifacts (`build/`, `*.o`, `*.a`)
- Compiled binaries (`vframetest`, `*.exe`)
- Binary releases (stored separately)
- Test data and temporary files
- IDE and editor configurations
- macOS system files (`.DS_Store`)

## Build Output

After compilation, the following are created (not in git):
- `build/vframetest` - Main executable
- `build/libvframetest.a` - Static library
- `build/*.o` - Object files
- Platform-specific builds in `build/win/`, `build/win64/`, etc.

## Module Organization

### Layering

```
frametest.c (Main entry point)
    ↓
tester.c (Test logic & result aggregation)
    ├→ profile.c (Frame profiles)
    ├→ frame.c (Frame operations)
    ├→ timing.c (Timing utilities)
    ├→ report.c (Output formatting)
    ├→ histogram.c (Performance visualization)
    └→ platform.c (Platform-specific operations)
```

### Platform-Specific Code

Platform abstraction through `platform.c/h`:
- **macOS**: Direct I/O, filesystem detection via statfs()
- **Linux**: Direct I/O, filesystem detection via magic numbers
- **Windows**: Direct I/O, filesystem detection via GetVolumeInformation()

## Configuration & Versioning

- **Version**: Defined in Makefile (MAJOR, MINOR, PATCH)
- **Build Flags**: C99 standard with strict warnings (-Wall -Werror -Wpedantic)
- **Threading**: POSIX threads (pthread) on all platforms
- **Cross-compilation**: Supported via MinGW for Windows

## CI/CD Pipeline

**File**: `.github/workflows/ci.yml`

**Platforms**: macOS, Linux, Windows
**Architectures**: arm64, x86_64, i686
**Stages**:
1. Build (parallel matrix builds)
2. Test (basic functionality tests)
3. Artifact upload (pre-built binaries)
4. Release (automated GitHub release)

## Documentation Hierarchy

```
README.md (User overview)
├── Quick start guide
├── Installation options
└── Link to detailed docs
    │
    ├── docs/FEATURES_GUIDE.md (All capabilities)
    ├── docs/PROJECT_EVOLUTION.md (Project history)
    ├── BUILD.md (Build instructions)
    ├── CONTRIBUTING.md (For developers)
    │
    └── wiki/ (Accessible via GitHub wiki)
        ├── Home.md
        ├── Features-Guide.md
        ├── Project-Evolution.md
        └── Phase-1-Completion.md
```

## Release & Distribution

### Official Releases
- **Location**: GitHub Releases
- **Artifacts**: Pre-built binaries for all platforms
- **Documentation**: Release notes with changelog

### Package Distribution
- **macOS**: Homebrew tap (ssotoa70/vframetest)
- **Linux**: Source builds or pre-built binaries
- **Windows**: Pre-built executables

### Source Distribution
- **Format**: tar.gz archives
- **Command**: `make dist` generates source tarball

## Future Structure Notes

As the project grows:
- Consider moving source files to `src/` directory (requires Makefile update)
- Platform-specific code could move to `platform/` subdirectory
- Could separate core library from CLI tool
- May benefit from additional test organization

---

**Last Updated**: November 25, 2025
**Repository**: https://github.com/ssotoa70/vframetest
