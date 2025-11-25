# vframetest - Multi-Platform Build Guide

vframetest is a cross-platform test and benchmark tool for media frame write/read operations. This guide covers building for macOS, Linux, and Windows.

## Quick Start

### macOS (Universal Binary)
```bash
# Build for current architecture (arm64 or x86_64)
make

# Build universal binary (both arm64 and x86_64)
./build_all.sh
```

### Linux
```bash
# Build native binary
make

# Or use the dedicated script
./build_linux.sh
```

### Windows
```bash
# Using MinGW toolchain (requires installation)
./build_windows.sh

# Or build manually
BUILD_FOLDER="build-win" CROSS=x86_64-w64-mingw32- ./build_win.sh
```

## Platform-Specific Build Instructions

### macOS

#### Prerequisites
- Xcode Command Line Tools (includes clang and make)
- Optional: Homebrew for additional tools

#### Building for Single Architecture
```bash
# Build for Apple Silicon (arm64)
make clean && make

# Build for Intel (x86_64)
CFLAGS="-arch x86_64 -std=c99 -O2 -Wall -Werror -Wpedantic -pedantic-errors -DMAJOR=3025 -DMINOR=10 -DPATCH=2" \
LDFLAGS="-arch x86_64 -pthread" \
make
```

#### Building Universal Binary (Recommended)
```bash
./build_all.sh
# Produces: releases/vframetest-3025.10.2/vframetest-*-macos-*
```

The universal binary works on both Apple Silicon and Intel Macs:
```bash
# Run on any Mac
./vframetest-3025.10.2-macos-universal --version
```

#### macOS-Specific Features
The macOS build includes Apple-specific optimizations:
- **F_NOCACHE**: Native direct I/O instead of faked O_DIRECT
- **F_FULLFSYNC**: Ensures data reaches physical disk
- **Proper memory alignment**: Correct posix_memalign error handling

### Linux

#### Prerequisites
**RHEL/CentOS/Fedora:**
```bash
sudo yum install gcc make libc6-dev  # For RHEL 8+: gcc make glibc-devel
```

**Ubuntu/Debian:**
```bash
sudo apt-get install build-essential
```

#### Building
```bash
# Native build
make clean && make

# Or use the dedicated script
./build_linux.sh
```

#### Cross-Compilation (Optional)
If you have cross-compilation toolchains installed:

**For ARM64 (from x86_64 host):**
```bash
CC=aarch64-linux-gnu-gcc AR=aarch64-linux-gnu-ar \
make clean && make
```

### Windows

#### Prerequisites
- **MinGW-w64** toolchain installed

**Installation:**
```bash
# macOS (Homebrew)
brew install mingw-w64

# Linux (Ubuntu/Debian)
sudo apt-get install mingw-w64

# Linux (RHEL/CentOS)
sudo yum install mingw64-gcc mingw32-gcc
```

#### Building
```bash
# 64-bit binary
BUILD_FOLDER="build-win" CROSS=x86_64-w64-mingw32- ./build_win.sh

# 32-bit binary
BUILD_FOLDER="build-win32" CROSS=i686-w64-mingw32- ./build_win.sh

# Or use the dedicated script
./build_windows.sh
```

#### Output
- `vframetest.exe` - Windows executable

## Build Output

After building, binaries are located in:
- `build/vframetest` - Current native build
- `vframetest-arm64` - macOS ARM64
- `vframetest-x86_64` - macOS Intel
- `vframetest-universal` - macOS universal
- `releases/vframetest-VERSION/` - Multi-platform releases

## Makefile Targets

```bash
make                    # Build for current platform
make clean             # Remove build artifacts
make release           # Strip debug symbols
make test              # Run unit tests
make coverage          # Generate coverage report
make format            # Format code with clang-format
make dist              # Create distribution tarball
make win               # Cross-compile for Windows (32-bit)
make win64             # Cross-compile for Windows (64-bit)
```

## Build Flags

### Common Flags
```bash
# Optimize for performance
CFLAGS="-O3" make

# Debug build (no optimizations)
CFLAGS="-O0 -g" make

# Custom installation prefix
PREFIX=/usr/local make install
```

### Architecture-Specific
```bash
# ARM64 (Apple Silicon)
make

# x86_64 (Intel/AMD)
CFLAGS="-arch x86_64 ..." make

# ARM64 Linux (cross-compile)
CC=aarch64-linux-gnu-gcc AR=aarch64-linux-gnu-ar make
```

## Troubleshooting

### "O_DIRECT not available" on macOS
This is expected. The macOS build uses F_NOCACHE instead, which provides equivalent functionality.

### "posix_memalign failed" on Linux
Ensure you have sufficient memory available and that your buffer alignment requirements are reasonable (typically 4KB or 64KB).

### Windows build fails with "mingw not found"
Install MinGW: `brew install mingw-w64` (macOS) or `sudo apt-get install mingw-w64` (Linux)

### Compilation errors on older systems
Try updating compiler or using older compiler flags:
```bash
CFLAGS="-std=c99 -O2 -Wall" make
```

## Development

### Code Style
```bash
# Format code
make format

# Check formatting
clang-format -i *.c *.h
```

### Running Tests
```bash
# Build and run tests
make test

# Coverage report
make coverage
```

## Release Process

```bash
# Build all platforms
./build_all.sh

# Create distributable archive
make dist

# Archives available in releases/vframetest-VERSION/
```

## Platform Matrix

| Platform | Architecture | Status | Notes |
|----------|--------------|--------|-------|
| macOS | arm64 (Apple Silicon) | ✓ Native | F_NOCACHE support |
| macOS | x86_64 (Intel) | ✓ Native | F_NOCACHE support |
| macOS | Universal | ✓ Native | Both architectures |
| Linux | x86_64 | ✓ Native | O_DIRECT support |
| Linux | ARM64 | ✓ Cross-compile | O_DIRECT support |
| Windows | x86_64 | ✓ Cross-compile | MinGW required |
| Windows | i686 | ✓ Cross-compile | MinGW required |

## Advanced Usage

### Custom Build Directory
```bash
BUILD_FOLDER=/tmp/mybuild make
```

### Separate Source and Build
```bash
mkdir build && cd build
cmake .. && make
```

### Install Binary
```bash
sudo make install PREFIX=/usr/local
```

## License

vframetest is licensed under GNU General Public License v2 or later.
See COPYING file for details.

## Contributing

To improve builds or add platform support:
1. Fork the repository
2. Create a feature branch
3. Make changes
4. Test on target platform
5. Submit pull request

---

For more information, see README.md and the main documentation.
