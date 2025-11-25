# Architecture & Technical Design

## Overview

vframetest is a modular, platform-abstracted C application that simulates video frame I/O to benchmark storage performance.

## Module Organization

```
frametest (main entry point)
├── profile → Frame profile definitions
├── tester → Test logic & aggregation
├── frame → Frame data structures
├── timing → High-resolution timing
├── report → Output formatting (CSV, JSON)
├── histogram → Performance visualization
└── platform → Platform abstraction (OS-specific)
```

## Key Design Principles

### 1. Platform Abstraction
- Single codebase for macOS, Linux, Windows
- Platform-specific code isolated in `platform.c`
- Feature detection at runtime (direct I/O, filesystem type)

### 2. Modular Architecture
- Clear separation of concerns
- Each module has well-defined interface (*.h)
- Minimal coupling between modules

### 3. Error Handling
- Comprehensive errno capture
- Frame-level error tracking
- Graceful degradation on errors
- Non-blocking error reporting

### 4. Performance
- Direct I/O when available
- Thread-safe result aggregation
- Efficient memory management
- Minimal synchronization overhead

## Core Components

### frametest.c/h
- Main driver and test orchestration
- User input parsing and validation
- Output formatting and display
- Filesystem detection integration

### tester.c/h
- Test execution engine
- Multi-threaded test coordination
- Result aggregation (critical for accuracy)
- Frame success/failure tracking

### platform.c/h
- OS-specific functionality
  - Direct I/O flags (F_NOCACHE, O_DIRECT)
  - Filesystem detection
  - Timing utilities
- Conditional compilation for each platform

### report.c/h
- Output format generation
- CSV export for spreadsheets
- JSON export for automation
- Flexible output pipeline

### profile.c/h
- Predefined frame profiles (SD, HD, 4K, 8K)
- Custom profile support
- Frame size calculations

## Data Structures

### test_result_t
```c
typedef struct {
    int frames_failed;
    int frames_succeeded;
    double success_rate_percent;
    filesystem_type_t filesystem_type;
    // ... performance metrics
} test_result_t;
```

### error_info_t
```c
typedef struct {
    int errno_value;
    char error_message[256];
    const char *operation;
    int frame_number;
    int thread_id;
    uint64_t timestamp;
} error_info_t;
```

## Build System

### Makefile Organization
- Version management (MAJOR.MINOR.PATCH)
- Multi-platform build targets
- Strict compiler flags (-Wall -Werror -Wpedantic)
- Platform-specific build scripts (Windows cross-compilation)

### Directory Structure (Post-Reorganization)
- `src/` - All source files (*.c, *.h)
- `scripts/build/` - Build automation scripts
- `tests/` - Unit and integration tests
- `docs/` - Documentation
- `examples/` - Usage examples
- `config/` - Configuration templates
- `assets/` - Images, diagrams, logos

## Threading Model

### Multi-threaded Testing
1. Main thread spawns worker threads
2. Each worker runs independent test loop
3. Workers write/read frames in parallel
4. Atomic operations for error counting
5. Main thread aggregates results

### Result Aggregation
Critical function: `test_result_aggregate()`
- Accumulates frame counters from threads
- Recalculates success rate
- Preserves timing information

## Platform-Specific Implementation

### macOS
- `statfs()` for filesystem detection
- `f_fstypename` for filesystem name
- `F_NOCACHE` for direct I/O
- Universal binary support (arm64 + x86_64)

### Linux
- `statfs()` with magic number matching
- Filesystem detection via `/etc/mtab` or proc
- `O_DIRECT` flag for direct I/O
- POSIX threads for multi-threading

### Windows
- `GetVolumeInformation()` for filesystem type
- UNC path detection for SMB
- `FILE_FLAG_NO_BUFFERING` for direct I/O
- MinGW cross-compilation support

## Performance Considerations

### Direct I/O
- Bypasses OS cache for accurate measurements
- Not available on all remote filesystems
- Fallback to buffered I/O if unavailable (Phase 2 goal)

### Memory Management
- Pre-allocated frame buffers
- Dynamic error array expansion (10→20→40)
- Proper cleanup via test_result_free()

### Timing
- High-resolution timers (nanosecond precision)
- Per-frame timing tracked separately
- Completion time statistics (min/avg/max)

## Error Tracking Pipeline

1. I/O operation fails → errno captured
2. Error recorded in dynamic array
3. Frame marked as failed
4. Success rate recalculated
5. Output includes error summary
6. Phase 2: Detailed error export

## CI/CD Integration

### GitHub Actions
- Matrix builds for all platforms/architectures
- Artifact isolation per platform
- Automated benchmark data collection
- Pre-built binary generation
- Automated release creation

### Build Artifacts
- macOS: universal, arm64, x86_64 binaries
- Linux: x86_64 binary
- Windows: x86_64, i686 executables

## Quality Assurance

### Code Standards
- C99 compliance
- Zero warnings policy (-Wall -Werror)
- Memory leak checking
- Thread safety verification

### Testing
- Unit tests in `tests/`
- Platform-specific test coverage
- Performance regression detection
- Functional validation across platforms

## Future Architecture Improvements (Phase 2+)

1. **Graceful I/O Fallback** - Auto-detect and fall back from direct I/O
2. **Error Export** - Detailed error logs in CSV/JSON format
3. **Configuration File Support** - YAML/JSON test profiles
4. **Plugin System** - Custom output formatters
5. **Web Dashboard** - Real-time performance monitoring

---

**For More Information**: See [Repository Structure](REPOSITORY_STRUCTURE.md) for file organization
