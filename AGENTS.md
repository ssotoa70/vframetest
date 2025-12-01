# vframetest - Agent Development Guide

This document helps AI agents work effectively in the vframetest repository by providing essential commands, patterns, conventions, and context.

## Project Overview

**vframetest** is a professional frame-based storage I/O benchmark tool evolved from the original Tuxera/SGI implementation. It tests actual video frame I/O patterns for media production workflows and data center infrastructure.

- **Language**: C99 (strict standards compliance)
- **Platforms**: macOS, Linux, Windows (cross-platform POSIX with MinGW)
- **Architecture**: Multi-threaded performance testing with comprehensive error tracking
- **License**: GPL v2.0

## Essential Commands

### Build Commands
```bash
# Clean build
make clean && make -j4

# Development build (includes debug symbols)
make clean && make CFLAGS="-std=c99 -g -O0 -Wall -Werror"

# Release build (stripped binary)
make release

# Cross-platform builds
make win64        # Windows 64-bit (requires MinGW)
make win          # Windows 32-bit
```

### Testing Commands
```bash
# Run unit tests
make test

# Run specific test manually
./build/tests/test_frame
./build/tests/test_tui

# Quick manual test
./build/vframetest -w FULLHD-24bit -t 4 -n 100 test-data

# Test with different output formats
./build/vframetest -c -w FULLHD-24bit -t 2 -n 500 /mnt/storage > results.csv
./build/vframetest -j -w FULLHD-24bit -t 4 -n 100 /mnt/storage > results.json
```

### Code Quality Commands
```bash
# Format all source code
make format

# Manual formatting check
clang-format --dry-run --Werror src/*.c src/*.h tests/*.c

# Generate coverage report
make coverage

# Clean build artifacts
make clean
```

### Distribution Commands
```bash
# Create source tarball
make dist

# Build all platform releases
make win && make win64  # Windows releases
```

## Code Organization & Structure

### Source Directory Layout
```
src/
├── Core Modules:
│   ├── frametest.c/h      # Main application entry point
│   ├── tester.c/h         # Test execution engine
│   ├── frame.c/h          # Frame data structures
│   ├── profile.c/h        # Video profile definitions (SD/HD/4K/etc)
│   └── platform.c/h       # Platform abstraction layer
├── I/O & Performance:
│   ├── timing.c/h         # High-precision timing utilities
│   ├── histogram.c/h      # Performance distribution analysis
│   └── report.c/h         # Result formatting (text/CSV/JSON)
├── TUI System (Phase 4):
│   ├── tui.c/h            # Main TUI coordinator
│   ├── tty.c/h            # Terminal control (raw mode, signals)
│   ├── tui_state.c/h      # Application state machine
│   ├── tui_input.c/h      # Keyboard input handling
│   ├── tui_views.c/h      # View rendering (dashboard/history/latency/config)
│   ├── tui_render.c/h     # TUI rendering utilities
│   └── screen.c/h         # Screen management
└── Legacy: tui_input.c/h  # Will be replaced by new system
```

### Build System
- **Primary Makefile**: Root directory for main application
- **Test Makefile**: `tests/Makefile` for unit tests with separate build folder
- **Build Output**: `build/` directory with separate subdirectories for tests/coverage
- **Version Management**: Version defined via -D flags (MAJOR, MINOR, PATCH)

### Module Dependencies
```
frametest.c → tester.c → platform.c → frame.c → profile.c
             ↓           ↓           ↓         ↓
           timing.c   histogram.c  report.c  tui*.c
```

## Coding Conventions & Style

### Code Formatting
- **Style**: Strict C99 with pedantic warnings
- **Indentation**: 8 spaces, tabs (`.clang-format`: `UseTab: Always`, `TabWidth: 8`)
- **Line Limit**: 80 characters (`ColumnLimit: 80`)
- **Braces**: Function braces on new line, control statements same line
- **Pointers**: Right-aligned (`char *ptr` not `char* ptr`)

### Naming Conventions
- **Types**: `snake_case_t` for typedefs (e.g., `test_result_t`, `frame_t`)
- **Functions**: `snake_case()` (e.g., `tester_run_write()`, `platform_get()`)
- **Variables**: `snake_case` (e.g., `frames_written`, `thread_info`)
- **Constants**: `UPPER_CASE` (e.g., `SEC_IN_NS`, `FILESYSTEM_LOCAL`)
- **Headers**: Include guards `FRAMETEST_MODULE_H` pattern

### Error Handling Patterns
```c
// Standard function return pattern
int function_name(params) {
    if (!param) {
        return -1;  // Error code
    }
    // Success
    return 0;
}

// Result structure pattern
typedef struct result_t {
    int success;              // 0=success, non-zero=error
    char error_msg[256];      // Human-readable error
    // ... result data
} result_t;
```

### Memory Management
- Use platform abstraction: `platform->malloc()`, `platform->free()`
- Always check return values
- Use `result_free()` helper functions for complex structures
- Error tracking with `error_info_t` structures

## Testing Approach

### Unit Test Framework
- **Custom Framework**: Lightweight `unittest.h` with assertion macros
- **Test Pattern**: `test_MODULE()` functions with `TEST_MAIN()` wrapper
- **Mock Platform**: `test_platform_get()` for isolated testing
- **Coverage**: Use `make coverage` for lcov reports

### Test Categories
- **frame**: Frame data structure operations
- **histogram**: Performance distribution calculations
- **platform**: Platform abstraction layer
- **profile**: Video profile validation
- **tester**: Test execution engine
- **tui**: Terminal user interface components

### Test Execution
```bash
# All tests
make test

# Individual test modules
cd tests && make test_frame
cd tests && make test_tui
```

## Important Gotchas & Non-Obvious Patterns

### Platform Abstraction
- Always use `platform->*` functions for memory, threads, timing
- Platform detection via `platform_get()` - don't use `#ifdef` in application code
- Filesystem detection automatic (`FILESYSTEM_LOCAL`, `FILESYSTEM_SMB`, etc.)

### Multi-Threading
- Worker threads use `thread_info_t` structures
- Shared progress state with `volatile` variables for TUI updates
- Pause/resume using condition variables (Phase 4 feature)

### Version Management
- Version numbers set via Makefile -D flags, not in source code
- Fallback version definitions in headers for IDE support
- Use `MAJOR`, `MINOR`, `PATCH` macros in code

### Phase-Based Development
Project uses phased development approach:
- **Phase 1**: Error tracking and filesystem detection ✅
- **Phase 2**: I/O fallback and enhanced reporting ✅  
- **Phase 3**: NFS/SMB optimization detection ✅
- **Phase 4**: Interactive TTY dashboard 🚧 (in development)

### Direct I/O Handling
- Automatic detection of Direct I/O availability
- Graceful fallback to buffered I/O with tracking
- Remote filesystem optimization (skips Direct I/O on NFS/SMB)

### Signal Handling
- Use `sigaction()` instead of `signal()` for POSIX compliance
- Terminal management in TUI mode requires proper signal cleanup
- SIGWINCH handling for terminal resize detection

## Build-Specific Notes

### Compiler Flags
```makefile
CFLAGS+=-std=c99 -O2 -Wall -Werror -Wpedantic -pedantic-errors
```

### Test Build Differences
- Tests use `-O0 -g` for debugging
- Additional include paths: `-I.. -I../src`
- Platform-specific linker flags for symbol wrapping (Linux only)

### Cross-Platform Considerations
- **Windows**: MinGW required, separate build targets
- **macOS**: Universal binary support (arm64 + x86_64)
- **Linux**: Standard GNU toolchain

## Project-Specific Context

### Performance Focus
- Designed for video workload simulation, not generic I/O
- Frame-based testing with realistic media file patterns
- High-precision timing using `clock_gettime()` (POSIX timers)

### Enterprise Features
- Comprehensive error tracking with errno and context
- Multiple output formats: text, CSV, JSON, histograms
- Filesystem detection and remote storage optimization
- Professional CI/CD with multi-platform builds

### Interactive Mode (Phase 4)
- Full TTY interface with real-time dashboard
- Config menu for all test parameters
- Frame history and latency analysis views
- Pause/resume test execution

## Working with the TUI System

When modifying TUI components:
1. All terminal operations go through `tty.c`
2. State management in `tui_state.c`
3. View rendering in `tui_views.c`
4. Use alternate screen mode for clean terminal experience
5. Handle SIGWINCH for terminal resize
6. Always restore terminal state on exit

## Memory & Performance

### Large Data Handling
- Frame history: default 10,000 frames (configurable)
- Large result structures: use aggregation functions
- Memory allocation through platform abstraction
- Coverage builds use gcov for profiling

### Timing Precision
- Nanosecond precision using `clock_gettime()`
- POSIX timers enabled via `_POSIX_C_SOURCE` on Linux
- Platform-specific high-resolution clocks

This guide provides the essential context for understanding and contributing to vframetest. The codebase emphasizes professional C practices, cross-platform compatibility, and comprehensive performance measurement capabilities.