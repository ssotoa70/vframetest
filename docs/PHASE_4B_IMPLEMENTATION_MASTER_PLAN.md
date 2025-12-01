# Phase 4B: REST API & Data Export - Master Implementation Plan

**Document**: PHASE_4B_IMPLEMENTATION_MASTER_PLAN.md  
**Date**: 2025-12-01  
**Version**: 1.0  
**Status**: In Planning → Ready for Implementation

---

## Executive Summary

Phase 4B introduces a REST API for programmatic access to vframetest test metrics and results. This document provides the complete implementation roadmap.

### Goals

1. ✅ Design 12+ REST API endpoints
2. ✅ Create HTTP server (zero external dependencies)
3. ✅ Implement metrics data access layer
4. ✅ Integrate with CLI (--api-port flag)
5. ✅ Comprehensive testing
6. ✅ Complete documentation

### Timeline

- **Duration**: 5 weeks
- **Target Release**: v25.16.0
- **Estimated LOC**: 3,000-4,000 lines

---

## Phase 4B Architecture

### System Design

```
vframetest with REST API
├── Test Execution Engine (existing)
│   ├── Metrics Collection (shared)
│   └── TUI Display (Phase 4A)
│
├── HTTP Server (NEW - Phase 4B)
│   ├── Request Handler
│   ├── Route Dispatcher
│   ├── Endpoint Implementations
│   └── Response Formatter
│
└── Data Access Layer (NEW - Phase 4B)
    ├── Metrics Reader
    ├── History Buffer
    └── Export Formatter
```

### API Endpoint Structure

#### Status & Progress (7 endpoints)

1. **GET /api/v1/status** - Current test status
2. **GET /api/v1/progress** - Real-time progress
3. **GET /api/v1/metrics** - Live metrics (throughput, latency, IOPS)
4. **GET /api/v1/io-modes** - I/O mode statistics
5. **GET /api/v1/filesystem** - Filesystem detection info
6. **GET /api/v1/history** - Frame history with pagination
7. **GET /api/v1/summary** - Final results (after completion)

#### Control Endpoints (3 endpoints)

8. **POST /api/v1/test/pause** - Pause running test
9. **POST /api/v1/test/resume** - Resume paused test
10. **POST /api/v1/test/stop** - Stop test

#### Export Endpoints (2 endpoints)

11. **GET /api/v1/export/json** - Full JSON export
12. **GET /api/v1/export/csv** - Frame-by-frame CSV

### Data Models

#### StatusResponse
```json
{
  "test_id": "uuid",
  "status": "running|paused|completed|failed",
  "test_type": "write|read|stream",
  "profile": "4K-24bit",
  "target_path": "/mnt/storage",
  "thread_count": 8,
  "started_at": "2025-12-01T16:30:00Z",
  "estimated_completion": "2025-12-01T16:45:00Z"
}
```

#### MetricsResponse
```json
{
  "frames_completed": 500,
  "total_frames": 1000,
  "percentage": 50.0,
  "elapsed_seconds": 450,
  "eta_seconds": 450,
  "throughput": {
    "mbps": 850.5,
    "iops": 6.2
  },
  "latency": {
    "min_ms": 5.1,
    "avg_ms": 1108.9,
    "max_ms": 3882.5,
    "p50": 890.2,
    "p95": 2450.1,
    "p99": 3650.3
  },
  "io_modes": {
    "direct_io": 300,
    "buffered_io": 200,
    "fallback_count": 0
  },
  "success_rate": 100.0
}
```

---

## Implementation Phases (Week-by-Week)

### Week 1: HTTP Server Foundation

**Goal**: Build basic HTTP server infrastructure

**Tasks**:
- [ ] Create src/http_server.h/c
- [ ] Socket management (listen, accept, close)
- [ ] Basic HTTP request parsing
- [ ] Response building
- [ ] Error handling

**Files**:
- src/http_server.h/c (~400 lines)
- src/http_routes.h/c (~150 lines)
- tests/test_http_server.c (~100 lines)

**Tests**:
- Server startup/shutdown
- Connection handling
- Basic request parsing
- Response formatting

### Week 2: Data Access Layer

**Goal**: Create metrics access and formatting

**Tasks**:
- [ ] Create src/api_data.h/c
- [ ] Read-only metrics access
- [ ] Thread-safe data retrieval
- [ ] CSV/JSON formatters
- [ ] Pagination support

**Files**:
- src/api_data.h/c (~500 lines)
- src/api_json.h/c (~300 lines)
- src/api_csv.h/c (~200 lines)
- tests/test_api_data.c (~150 lines)

**Tests**:
- Metrics retrieval
- Thread safety
- Data formatting
- Edge cases

### Week 3: API Endpoints Implementation

**Goal**: Implement all 12 endpoints

**Tasks**:
- [ ] Status endpoints (7 endpoints)
- [ ] Control endpoints (3 endpoints)
- [ ] Export endpoints (2 endpoints)
- [ ] Error responses
- [ ] Input validation

**Files**:
- src/api_endpoints.h/c (~1,000 lines)
- tests/test_api_endpoints.c (~300 lines)

**Tests**:
- Each endpoint functionality
- Parameter validation
- Error handling
- Response formatting

### Week 4: CLI Integration

**Goal**: Integrate API into CLI

**Tasks**:
- [ ] Add --api-port flag
- [ ] Add --api-bind flag (optional)
- [ ] Server lifecycle management
- [ ] Graceful shutdown
- [ ] Conflict detection (TUI + API)

**Files**:
- src/frametest.c (modifications)
- src/frametest.h (modifications)
- Makefile (modifications)

**Tests**:
- Flag parsing
- Server lifecycle
- Parameter handling

### Week 5: Testing & Documentation

**Goal**: Complete testing and documentation

**Tasks**:
- [ ] Integration tests (API + test execution)
- [ ] Performance testing
- [ ] Load testing
- [ ] API documentation (OpenAPI/Swagger)
- [ ] Client library examples
- [ ] README updates

**Deliverables**:
- tests/test_api_integration.c (~250 lines)
- API specification (OpenAPI YAML)
- Client examples (curl, Python, JavaScript)
- User documentation

---

## Build System Updates

### Makefile Changes

```makefile
# Add HTTP server sources
SOURCES += http_server.c http_routes.c api_data.c api_json.c api_csv.c api_endpoints.c

# Add API flags (optional, for performance tuning)
# CFLAGS += -DAPI_MAX_CONNECTIONS=100
# CFLAGS += -DAPI_BUFFER_SIZE=8192
```

### Test Build Updates

```makefile
# Add API tests
TESTS += http_server api_data api_endpoints api_integration
```

---

## Performance Targets

| Metric | Target | Verification |
|--------|--------|--------------|
| Endpoint Response Time | <10ms | Benchmark test |
| Memory Overhead | <5MB | Valgrind check |
| CPU Overhead | <0.5% | Perf test |
| Max Concurrent Connections | 100+ | Load test |
| Data Throughput | 10+ MB/s | Throughput test |

---

## Integration Points

### Metrics Access

```c
// Read-only access to metrics
typedef struct {
    uint64_t frames_completed;
    uint64_t total_frames;
    uint64_t bytes_written;
    uint64_t elapsed_ns;
    // ... other metrics
} api_metrics_t;

api_metrics_t api_get_metrics(void);
api_history_t* api_get_history(size_t limit, size_t offset);
```

### Control Points

```c
// Pause/resume/stop (implemented in Phase 4A)
int api_pause_test(void);
int api_resume_test(void);
int api_stop_test(void);
```

---

## Risk Analysis

### Identified Risks

| Risk | Likelihood | Impact | Mitigation |
|------|-----------|--------|-----------|
| Port conflicts | Medium | Medium | Check/configure port |
| Network issues | Low | Low | Localhost only (v1) |
| Data race conditions | Low | High | Mutex protection |
| Memory leaks | Low | Medium | Valgrind testing |
| Performance regression | Low | Medium | Baseline measurements |

---

## Success Criteria

### Functional

- ✅ All 12 endpoints implemented
- ✅ All control operations work
- ✅ Export formats correct
- ✅ Error handling complete

### Non-Functional

- ✅ Zero memory leaks
- ✅ <0.5% performance overhead
- ✅ 100+ tests passing
- ✅ Zero compiler warnings

### Documentation

- ✅ API specification complete
- ✅ Client examples provided
- ✅ User guide written
- ✅ Wiki updated

---

## Deliverables Summary

### Code Files (15-20 new/modified)

- HTTP Server: src/http_server.h/c, src/http_routes.h/c
- Data Layer: src/api_data.h/c, src/api_json.h/c, src/api_csv.h/c
- Endpoints: src/api_endpoints.h/c
- Integration: frametest.c, frametest.h, Makefile
- Tests: tests/test_*.c (6 test files)

### Documentation

- OpenAPI/Swagger specification
- API user guide
- Client examples (3+ languages)
- Integration guide for Phase 5

### Release Artifacts

- v25.16.0 tag
- GitHub release with notes
- Updated CHANGELOG
- Updated README
- Updated Wiki (new API page)

---

## Next Phase: Phase 4C

After Phase 4B completion:

**Phase 4C**: Enhanced Output Formats & Refinements
- Advanced export formats
- Performance analytics
- Automated report generation
- Monitoring integration

**Phase 5 Vision**: Web Dashboard
- Use Phase 4B REST API
- Modern web interface
- Real-time visualization
- Historical analysis

---

## Approval & Sign-Off

| Role | Status | Date |
|------|--------|------|
| **Plan Author** | Ready | 2025-12-01 |
| **Architecture Review** | Pending | - |
| **Implementation Ready** | Yes | 2025-12-01 |

---

**Document Status**: READY FOR IMPLEMENTATION ✅

This plan will be executed starting immediately after Phase 4A release (v25.15.0).

