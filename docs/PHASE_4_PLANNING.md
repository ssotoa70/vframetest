# Phase 4 Planning: TUI-First Real-Time Monitoring

**Status**: Planning Phase (Not Yet Implemented)
**Decision Date**: 2025-11-28
**Architecture Approach**: TUI-First with REST API

---

## Overview

Phase 4 introduces real-time test monitoring through a Terminal User Interface (TUI), enabling engineers to observe test progress live without external dependencies. The TUI-first approach aligns with vframetest's CLI heritage and serves DevOps/SysAdmin workflows efficiently.

---

## Phase 4 Breakdown

### Phase 4A: TUI Real-Time Dashboard (Priority 1)

**Objective**: Provide live test monitoring in the terminal

#### Features
- **Progress Display**
  - Frame count (current/total)
  - Time elapsed and ETA
  - Completion percentage

- **Performance Metrics**
  - Real-time throughput (MiB/s, FPS)
  - Frame time statistics (min/max/avg)
  - Performance trend (improving/stable/degrading)

- **I/O Optimization Status**
  - Direct I/O vs Buffered I/O count
  - Fallback events in real-time
  - Success rate (frames succeeded/failed)

- **Filesystem Information**
  - Detected filesystem type
  - Network timeout status
  - Optimization recommendations

- **Visual Indicators**
  - Progress bars (Unicode)
  - Trend arrows/sparklines
  - Color-coded status
  - Real-time chart updates

#### Technical Specifications
- **Dependencies**: Zero external (ANSI escape codes)
- **Implementation**: C language (native to vframetest)
- **Updates**: Non-blocking refresh (every 100ms)
- **Overhead**: <1% performance impact
- **Compatibility**: SSH, remote terminals, all emulators

#### Mock UI Design
```
┌──────────────────────────────────────────────────────────────┐
│ vframetest v25.13.0 - Real-Time Monitor    [q]uit [p]ause   │
├──────────────────────────────────────────────────────────────┤
│ Test: FULLHD-24bit (1920×1080×24) | Mode: WRITE              │
│ Path: /mnt/storage | Threads: 4                              │
├──────────────────────────────────────────────────────────────┤
│ Progress:       [████████░░░░░░░░░░░░░░░░░░░] 35% (350/1000) │
│ Elapsed: 2m 34s | ETA: 4m 46s                                │
├──────────────────────────────────────────────────────────────┤
│ Performance:                                                  │
│   Throughput: ████████░░  87.3 MiB/s                         │
│   Frame Rate: ████████░░  29.2 FPS                           │
│   Avg Frame:  7.2ms (min: 5.1ms, max: 12.3ms)               │
│   Trend: ↗ Improving                                         │
├──────────────────────────────────────────────────────────────┤
│ I/O Modes:                                                    │
│   Direct I/O:   ███░░░░░░░░░░░░░░░░░░░░  30.1% (105 frames) │
│   Buffered I/O: ███████░░░░░░░░░░░░░░░░  69.9% (245 frames) │
│   Fallbacks:    12 (Performance impact: minimal)             │
├──────────────────────────────────────────────────────────────┤
│ Filesystem & Optimization:                                    │
│   Type: NFS (Network detected)                               │
│   Status: ✓ Direct I/O skipped (optimized for NFS)          │
│   Timeout: 30s (configured)                                  │
│   Success Rate: 100.0% (350 succeeded, 0 failed)            │
├──────────────────────────────────────────────────────────────┤
│ System Status: Running (healthy)                              │
│ CPU: 15% | Memory: 42MB | Temp: 62°C                         │
└──────────────────────────────────────────────────────────────┘
```

### Phase 4B: REST API & Data Export (Priority 2)

**Objective**: Enable programmatic access to test results

#### API Endpoints
- `GET /api/v1/test/status` - Current test status
- `GET /api/v1/test/metrics` - Real-time metrics
- `GET /api/v1/test/results` - Complete test results
- `GET /api/v1/test/history` - Historical test data
- `POST /api/v1/test/pause` - Pause running test
- `POST /api/v1/test/resume` - Resume paused test

#### Data Format
```json
{
  "test_id": "uuid",
  "status": "running",
  "progress": {
    "frames_completed": 350,
    "total_frames": 1000,
    "percentage": 35.0,
    "elapsed_seconds": 154,
    "eta_seconds": 286
  },
  "metrics": {
    "throughput_mbps": 87.3,
    "fps": 29.2,
    "frame_time_ms": {
      "min": 5.1,
      "max": 12.3,
      "avg": 7.2
    },
    "trend": "improving"
  },
  "io_modes": {
    "direct_io": 105,
    "buffered_io": 245,
    "fallback_count": 12
  }
}
```

#### Implementation Notes
- Lightweight HTTP server (no external deps if possible)
- JSON output for easy parsing
- Real-time updates via polling or WebSocket
- TUI consumes this API internally

### Phase 4C: Enhanced Output Formats (Priority 3)

**Objective**: Improve integration with external monitoring

#### Features
- Structured export for analysis tools
- Performance degradation alerts
- Integration hooks for monitoring systems

---

## Architecture Decisions

### Why ANSI Escape Codes (No External Dependencies)

**Pros:**
- Zero external dependencies
- Works everywhere (any terminal)
- Lightweight and fast
- Full control over layout
- SSH-compatible

**Cons:**
- More manual code
- Terminal capability detection needed
- Some terminals have limitations

**Decision**: Implement custom ANSI terminal abstraction layer

### Why TUI Before Web Dashboard

1. **Immediate Value**: Users get live monitoring right away
2. **CLI Philosophy**: Aligns with Unix tools (htop, iotop, nethogs)
3. **Dev Efficiency**: TUI is faster to build than web stack
4. **Use Case Fit**: Storage engineers work in terminals
5. **Foundation**: REST API created in Phase 4B enables web dashboard in Phase 5

### Performance Considerations

- Non-blocking I/O for terminal updates
- <1% performance overhead target
- Refresh rate: 100-500ms (configurable)
- No impact on test execution

---

## Implementation Plan

### Phase 4A: TUI Dashboard Implementation Steps
1. Create terminal abstraction layer (ANSI codes)
2. Implement progress display
3. Add real-time metrics display
4. Implement performance trend visualization
5. Add filesystem optimization status
6. Implement keyboard controls (pause/resume/quit)
7. Integration with existing test execution
8. Cross-platform testing

### Phase 4B: REST API Implementation Steps
1. Lightweight HTTP server setup
2. API endpoint implementation
3. TUI integration with API
4. Real-time data export
5. Documentation and examples

### Phase 4C: Output Format Enhancement
1. Structured data export
2. Alert mechanism
3. External integration points

---

## Technical Specifications

### TUI Terminal Layer

```c
// Proposed abstraction layer
struct terminal_t {
    int width;
    int height;
    int supports_color;
    // Functions for drawing
    void (*clear)(void);
    void (*move_cursor)(int row, int col);
    void (*set_color)(int fg, int bg);
    void (*draw_box)(int x, int y, int w, int h);
    void (*draw_progress)(float percent);
    // etc.
};
```

### Data Flow
```
Test Execution
    ↓
Real-time Metrics Collection
    ↓
REST API (Phase 4B)
    ↓
TUI Dashboard (Phase 4A)
    ↓
Terminal Display
```

---

## Success Criteria

- ✅ TUI displays without external dependencies
- ✅ Real-time updates with <1% performance overhead
- ✅ Works over SSH connections
- ✅ All major terminal emulators supported
- ✅ Keyboard controls responsive
- ✅ REST API functional and documented
- ✅ All tests passing
- ✅ Zero regressions from Phase 3

---

## Dependencies & Constraints

### Dependencies
- None (ANSI escape codes are built-in to terminals)
- Standard C library only

### Constraints
- Terminal size detection needed
- Color support detection needed
- Escape sequence compatibility varies by terminal
- Performance must not impact test execution

---

## Timeline & Resources

**Phase 4A TUI**: ~2-3 weeks
**Phase 4B REST API**: ~1-2 weeks
**Phase 4C Output Formats**: ~1 week
**Testing & Polish**: ~1 week

---

## Risk Mitigation

| Risk | Mitigation |
|------|-----------|
| Terminal compatibility | Thorough testing on multiple terminals |
| Performance impact | Non-blocking I/O, minimal refresh rate |
| Complex ANSI codes | Well-documented abstraction layer |
| Maintenance burden | Clean API, good documentation |

---

## Future Expansion

After Phase 4 completion, the REST API opens doors for:
- Phase 5: Web dashboard
- Phase 6: Mobile app
- Phase 6: Integration with monitoring systems (Prometheus, Grafana)
- Advanced analytics and reporting

---

## Decision Log

**Decision**: TUI-First Approach
**Date**: 2025-11-28
**Rationale**:
- Aligns with vframetest's CLI nature
- Serves primary use case (live monitoring)
- Faster to implement than web stack
- No external dependencies
- Works over SSH (critical for servers)

**Decision**: No External Dependencies for TUI
**Date**: 2025-11-28
**Rationale**:
- ANSI codes are universally available
- Eliminates dependency bloat
- Simpler distribution
- Better performance

---

**Document Status**: Planning (Implementation Not Started)
**Last Updated**: 2025-11-28
**Next Step**: Implement Phase 4A TUI Dashboard
