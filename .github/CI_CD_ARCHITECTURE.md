# CI/CD Pipeline Architecture

## Overview

The vframetest CI/CD pipeline is a comprehensive, multi-platform build and deployment system that supports Linux, macOS, and Windows with multiple architectures and compiler variants.

## Pipeline Stages

### 1. **LINT** - Code Quality Checks
**Runs:** Once on `ubuntu-latest`
**Purpose:** Enforce code standards before building
- Code formatting check with `clang-format`
- Static analysis with `cppcheck`
- Reports generated as artifacts for review

**Key Features:**
- Non-blocking (continues even if issues found)
- Suppresses false positives (missing includes, unused functions)
- Generates detailed reports for developers

### 2. **BUILD** - Compile for All Platforms
**Runs:** In parallel across 7 build configurations
**Platforms:**
- **macOS:** 3 variants (universal, arm64, x86_64)
- **Linux:** 2 variants (gcc, clang)
- **Windows:** 2 variants (x86_64, i686)

**Key Features:**
- Matrix strategy for OS, architecture, and compiler combinations
- Platform-specific dependency installation and caching
- Universal binary creation for macOS (arm64 + x86_64)
- Smoke tests to verify binary functionality
- Build timing and metadata collection
- OS/arch-aware artifact naming

### 3. **TEST** - Run Comprehensive Tests
**Runs:** On 3 primary platforms (Linux, macOS, Windows x86_64)
**Tests:**
- Unit tests via `make test`
- Memory leak detection (Linux only, using valgrind)
- Platform-specific test execution

**Key Features:**
- Tests run after successful builds
- Separate test results per platform
- Non-blocking memory checks on Linux
- Artifact upload for test reports

### 4. **PACKAGE** - Create Release Artifacts
**Runs:** When tag is pushed (format: `v*`)
**Purpose:** Organize and checksum all binaries
- Collects artifacts from all build jobs
- Generates SHA256 checksums
- Creates release notes with artifact descriptions

**Key Features:**
- Automatic trigger on git tags
- Multi-platform artifact organization
- Checksum verification
- Detailed release notes generation

### 5. **DEPLOY** - Publish GitHub Release
**Runs:** After successful packaging (tags only)
**Purpose:** Create official GitHub release
- Downloads all packaged artifacts
- Validates artifacts before release
- Creates GitHub release with checksums
- Publishes release notes

**Key Features:**
- Artifact validation before publishing
- Checksum verification
- Beta/alpha detection from tag name
- Secure token handling

### 6. **STATUS-CHECK** - Pipeline Verification
**Runs:** After all primary jobs complete
**Purpose:** Verify entire pipeline success
- Aggregates job results
- Fails pipeline if any critical job failed
- Provides clear success/failure status

## Matrix Strategies

### Build Matrix
```
OS          | Architecture | Compiler | Configuration
------------|--------------|----------|---------------
macOS       | universal    | clang    | arm64+x86_64
macOS       | arm64        | clang    | native
macOS       | x86_64       | clang    | native
Linux       | x86_64       | gcc      | native
Linux       | x86_64       | clang    | native
Windows     | x86_64       | mingw64  | native
Windows     | i686         | mingw32  | cross-compile
```

### Test Matrix
```
OS          | Architecture | Configuration
------------|--------------|---------------
Linux       | x86_64       | gcc native
macOS       | x86_64       | clang native
Windows     | x86_64       | mingw64 native
```

## Environment Management

### Global Variables
```yaml
RUST_BACKTRACE: 1        # Enable detailed panic backtraces
CARGO_INCREMENTAL: 0     # Disable incremental builds
```

### Platform-Specific Variables
- **macOS:** `CC=clang`, `LDFLAGS=-pthread`
- **Linux:** `CC=gcc|clang`, `LDFLAGS=-pthread`
- **Windows:** `CC=mingw64|mingw32`, `CROSS_COMPILE=i686-w64-mingw32-` (i686 only)

### Build Environment Setup
- Dynamic version reading from Makefile
- Platform-specific build directory organization
- Artifact naming: `vframetest-{os}-{arch}`

## Caching Strategy

### macOS
- Homebrew package cache
- Key: `${{ runner.os }}-brew-${{ hashFiles('.github/workflows/ci.yml') }}`
- Path: `~/Library/Caches/Homebrew`, `/Library/Caches/Homebrew`

### Linux
- APT package cache
- Key: `${{ runner.os }}-apt-${{ hashFiles('.github/workflows/ci.yml') }}`
- Path: `~/.cache/apt`

### Strategy
- Cache keys depend on workflow file hash
- Updates cache when workflow changes
- Reduces dependency installation time by ~60%

## Artifact Management

### Build Artifacts
- **Name Pattern:** `vframetest-{os}-{arch}`
- **Retention:** 30 days
- **Includes:** Compiled binaries + build reports
- **Naming:** OS/arch aware for easy identification

### Test Artifacts
- **Name Pattern:** `test-results-{os}-{arch}`
- **Retention:** 7 days
- **Includes:** Test output, coverage reports

### Release Artifacts
- **Name Pattern:** `vframetest-{version}-{os}-{arch}`
- **Retention:** 90 days
- **Includes:** Final binaries, checksums, release notes

## Dependency Handling

### macOS
- Homebrew: `cppcheck`, `clang-format`, `valgrind`
- Build: Clang (built-in)

### Linux
- APT: `build-essential`, `gcc`, `clang`, `cppcheck`, `clang-format`, `valgrind`
- Toolchains: GCC, Clang (via apt)

### Windows
- MSYS2: MinGW-W64 (x86_64 and i686), `make`, `cppcheck`
- Shells: MSYS2 bash for cross-platform compatibility

## Secret & Credential Handling

### Best Practices Implemented
1. **No hardcoded secrets** in workflow files
2. **GitHub GITHUB_TOKEN** used automatically
3. **Environment-scoped permissions:**
   - `lint`: `contents: read`
   - `build`: `contents: read`
   - `test`: `contents: read`
   - `package`: `contents: read`
   - `deploy`: `contents: write` (token access)

### Secure Practices
- Tokens passed via environment variables
- No credential logging in CI output
- Minimal permission elevation

## Cross-Platform Compatibility

### Shell Handling
- **POSIX shells:** macOS/Linux use `bash` directly
- **Windows:** Uses `msys2 {0}` for POSIX compatibility
- **Conditional logic:** Platform detection via `runner.os`

### Path Normalization
- Dynamic path resolution (no hardcoded paths)
- Platform-aware binary extensions (`.exe` on Windows)
- Build directory structure: `build-{artifact_suffix}`

### Version Management
- Single source of truth: Makefile (`MAJOR`, `MINOR`, `PATCH`)
- Dynamic version extraction in all jobs
- Version flags propagated to all binaries

## Smoke Tests

### Execution
All binaries verify with `--version` flag after build

### Purpose
- Verify binary is executable
- Confirm version is embedded correctly
- Catch linking issues early

### Benefits
- Quick verification (< 1 second per binary)
- Catches architecture/toolchain issues
- Provides early feedback on build problems

## Backwards Compatibility

### Preserved Behavior
✅ Builds compile same binaries as before
✅ Tests execute identically
✅ Artifacts available in same locations
✅ Version numbering unchanged
✅ Release process unchanged

### Migrations
- Old `cmake-multi-platform.yml` removed (was unused)
- All existing scripts and Makefiles work unchanged
- No changes required to source code

## Idempotency

### Guaranteed Idempotent Operations
- ✅ Clean builds (via `make clean`)
- ✅ Artifact uploads (overwrites on re-run)
- ✅ Version extraction (from Makefile)
- ✅ Cache operations (stateless with hash keys)
- ✅ Release creation (draft creation on duplicates)

### Design Decisions
- No external state dependencies
- All versions self-contained in repo
- Artifacts can be re-generated at any time
- Pipeline runs identically regardless of history

## Concurrency Management

```yaml
concurrency:
  group: ${{ github.workflow }}-${{ github.ref }}
  cancel-in-progress: true
```

**Behavior:**
- Cancels previous runs on same branch/PR
- Prevents duplicate work
- Saves CI minutes on rapid pushes
- Latest commit always completes

## Failure Handling

### Non-Blocking Steps
- Code formatting checks (`continue-on-error: true`)
- Memory leak detection (non-blocking)
- Benchmark collection (graceful fallback)

### Hard Failures
- Build compilation must succeed
- Unit tests must pass
- Binary verification required
- Package checksums validated

### Error Reporting
- Clear error messages in logs
- Artifact uploads on failure (if requested)
- Reports available for debugging

## Monitoring & Reporting

### Available Artifacts
1. **Lint Reports:** Code format and static analysis issues
2. **Build Reports:** Timing, status, platform info
3. **Test Results:** Test output and coverage data
4. **Release Notes:** Artifact descriptions and verification instructions

### Log Visibility
- Clear section headers (`=== Section Name ===`)
- Status indicators (✅/❌/⚠️)
- Detailed configuration output
- Build timing information

## Future Enhancements

Possible additions:
- [ ] Code coverage reports (lcov integration)
- [ ] Performance benchmarking dashboard
- [ ] Automated security scanning (SAST)
- [ ] Container image builds
- [ ] Automated version bumping
- [ ] Multi-region artifact mirrors
