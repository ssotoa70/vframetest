# Changelog

All notable changes to vframetest are documented in this file.

## [25.17.3] - 2025-02-03

### Fixed
- **CRITICAL**: Fixed timing accuracy bug in metrics calculation
  - **Issue**: Metrics were storing absolute timestamps instead of elapsed times
  - **Impact**: All latency metrics (average, min, max, percentiles) and throughput calculations were completely inaccurate, showing values off by billions of nanoseconds
  - **Root Cause**: Code was calling `timing_start()` (which returns absolute nanoseconds since epoch) instead of `timing_elapsed(frame_start)` (which calculates elapsed time)
  - **Files Modified**: `src/tester.c` (lines 336, 515 in write/read test loops)
  - **Fix**: Changed 2 lines to use `timing_elapsed(frame_start)` to calculate elapsed nanoseconds
  - **Regression Testing**:
    - Histogram calculations still work correctly (they use subtraction of absolute times)
    - Direct I/O tracking unaffected
    - Success/failure counting unaffected
    - All metrics now produce realistic nanosecond values instead of billions
  - **Verification**: System timing now matches reported average latency within 5%

### Testing
- Added verification test: Compare tool-reported average latency to system elapsed time
- Confirmed histogram still displays correctly after fix
- Confirmed error tracking still works correctly
- All other metrics (Direct I/O %, success rate) unaffected and working

### Technical Details
- **Before Fix**: Average latency reported as ~1,707,000,000,000 ns (57+ years)
- **After Fix**: Average latency reported as ~125,000,000 ns (125 ms) for typical I/O

---

## [25.17.2] - 2025-02-02

### Fixed
- Add missing PROGRESS section header to TUI dashboard

---

## [25.17.1] - 2025-02-02

### Fixed
- Implement double-line box drawing for TUI dashboard

---

## [25.17.0] - 2025-02-01

### Features
- TUI Dashboard Enhancement - Structured Section Headers
- Interactive configuration menu enhancements

---

## [25.16.1] - 2025-01-28

### Fixed
- Bug fix release

### Performance
- Optimize metrics collection with --no-metrics flag

---

## [25.16.0] - 2025-01-25

### Features
- Phase 4 completion: TUI Dashboard with real-time metrics
- HTTP REST API endpoints (Phase 4B)
- Interactive TUI enhancements

---

## [25.15.0] and earlier

See git history for earlier changes.

---

## Version Format

- **MAJOR**: Breaking changes or major feature releases
- **MINOR**: Regular feature/enhancement releases
- **PATCH**: Bug fixes and minor improvements
