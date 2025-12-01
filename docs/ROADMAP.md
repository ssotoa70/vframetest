# Project Roadmap

## Completed: Phase 1 ✅ (v25.11.23)

### Error Tracking & Diagnostics
- ✅ System errno capture and logging
- ✅ Frame-level error tracking
- ✅ Thread identification in errors
- ✅ Success rate metrics
- ✅ Human-readable error messages

### Filesystem Detection
- ✅ LOCAL/SMB/NFS/OTHER detection
- ✅ Platform-specific implementation (macOS, Linux, Windows)
- ✅ Direct I/O availability checking
- ✅ Remote filesystem warnings
- ✅ User-friendly output

### Infrastructure
- ✅ Multi-platform support (macOS, Linux, Windows)
- ✅ Universal binaries (macOS arm64 + x86_64)
- ✅ GitHub Actions CI/CD
- ✅ Package management (Homebrew)
- ✅ Comprehensive documentation (2000+ lines)

## Completed: Phase 2 ✅ (v25.12.0)

### Graceful I/O Fallback
- ✅ Direct I/O → Buffered I/O automatic fallback
- ✅ Result annotation showing actual I/O mode used
- ✅ Fallback statistics and tracking
- ✅ Per-frame I/O mode recording

### Enhanced Error Reporting
- ✅ CSV export of error details with error_frame, error_operation, error_errno
- ✅ JSON error log generation with timestamps
- ✅ Detailed error breakdown reports (per-operation)
- ✅ Error statistics aggregation

### Output Enhancements
- ✅ Filesystem type in CSV/JSON exports
- ✅ Success rate in all output formats
- ✅ Detailed error statistics sections
- ✅ I/O fallback statistics in outputs

## Completed: Phase 3 ✅ (v25.13.0)

### NFS/SMB Optimization Detection
- ✅ Automatic filesystem type detection at startup
- ✅ Direct I/O skipping on remote filesystems (NFS/SMB)
- ✅ Network timeout handling (30-second default)
- ✅ Performance trend analysis (improving/stable/degrading)
- ✅ Enhanced CSV output with 6 new columns
- ✅ Enhanced JSON output with optimization_metrics section
- ✅ Cross-platform support (macOS, Linux, Windows)

## Completed: Professional Frame Formats ✅ (v25.13.1)

### DPX/EXR Profile Addition & Profile Filtering
- ✅ DPX Profiles (10/12-bit professional uncompressed)
  - DPX-2K-10bit (2048×1556, 4 bytes/pixel, 8KB header)
  - DPX-FULLHD-10bit (1920×1080, 4 bytes/pixel, 8KB header)
  - DPX-4K-10bit (3840×2160, 4 bytes/pixel, 8KB header)
  - DPX-8K-10bit (7680×4320, 4 bytes/pixel, 8KB header)
- ✅ EXR Profiles (professional float/half-precision)
  - Half-precision: EXR-FULLHD-half, EXR-4K-half, EXR-8K-half (6 bytes/pixel)
  - Float-precision: EXR-FULLHD-float, EXR-4K-float, EXR-8K-float (12 bytes/pixel)
- ✅ Profile Filtering - `--list-profiles-filter <prefix>` for quick profile lookup
- ✅ Enhanced Byte Size Display - Raw and aligned sizes in `--list-profiles` output
- ✅ Comprehensive Testing - New unit tests for all profiles
- ✅ Version bumped to 25.13.1

## Completed: Phase 4A ✅ (v25.14.0)

### Phase 4A: TUI Real-Time Dashboard
**Terminal User Interface for Live Test Monitoring**
- ✅ Real-time test progress and performance metrics
- ✅ Live performance trends (improving/stable/degrading)
- ✅ I/O mode visualization (Direct vs Buffered)
- ✅ Filesystem optimization status display
- ✅ Unicode-based charts and visualizations
- ✅ No external dependencies (ANSI escape codes)
- ✅ Works over SSH connections
- ✅ Keyboard controls for navigation
- ✅ Modular architecture (TUI core, state, render, input)
- ✅ Thread-safe atomic operations
- ✅ <1% performance overhead
- ✅ Comprehensive unit tests
- ✅ Cross-platform support (macOS, Linux, Windows)

## In Development: Phase 4B & 4C 🚀

### Phase 4B: REST API & Data Export (Priority 2)
**Programmatic Access to Test Results**
- RESTful API for test result queries
- Real-time metrics via WebSocket (optional)
- CSV/JSON export from API
- Query historical data
- Integration with external tools
- TUI consumes this API internally

### Phase 4C: Enhanced Output Formats (Priority 3)
- Structured data export for analysis
- Integration with monitoring systems
- Alert triggers on performance degradation

## Future Vision 🔮

### Phase 5: Web Dashboard & Historical Analysis
- Rich web-based visualization
- Historical data deep-dives
- Multi-test comparison
- Team collaboration features
- Automated performance reports
- Consumed REST API from Phase 4B

### Phase 6: Advanced Analytics & Cloud Integration
- Machine learning anomaly detection
- AWS/Azure/GCP performance tracking
- Distributed testing orchestration
- Automated optimization recommendations

### Phase 7+: Enterprise Features
- Multi-user management
- Advanced alerting & notifications
- Custom reporting
- Integration with ITSM systems

### Advanced Features
- Custom test profile wizard
- Performance baseline generation
- Stress testing mode
- Endurance testing suite

## Release Schedule

- **Phase 1**: ✅ Released (v25.11.23)
- **Phase 2**: Projected early 2026
- **Phase 3**: Projected mid 2026
- **Beyond**: Ongoing enhancements

## Community Roadmap

We welcome community input on:
- Feature requests via [GitHub Issues](https://github.com/ssotoa70/vframetest/issues)
- Enhancement ideas via GitHub Discussions
- Bug reports for prioritization
- Use case documentation

See [GOVERNANCE.md](GOVERNANCE.md) for decision-making process.

---

**Latest Status**: Phase 1 Complete ✅ | Phase 2 Complete ✅ | Phase 3 Complete ✅ | Phase 4 Planned
**Version**: 25.13.0 (Phase 3 Release)
**For Details**: See [PROJECT_EVOLUTION.md](PROJECT_EVOLUTION.md) and [PHASE_3_COMPLETION_REPORT.md](PHASE_3_COMPLETION_REPORT.md)
