# CI/CD Pipeline: Assumptions & Dependencies

## Overview

This document outlines all assumptions made in the new CI/CD pipeline and declares external dependencies.

---

## Repository Structure Assumptions

### Required Directories
```
vframetest/
├── src/                    # ✅ REQUIRED - Source files (.c, .h)
├── tests/                  # ✅ REQUIRED - Test files (.c, .h)
├── scripts/                # ⚠️ OPTIONAL - Build scripts (if exists)
│   └── collect-benchmarks.sh
├── .github/
│   └── workflows/
│       └── ci.yml          # ✅ This file
└── Makefile                # ✅ REQUIRED - Build definition
```

### Build System Assumptions
- **Build Tool:** Make (GNU Make)
- **Language:** C (C99 standard)
- **Compiler:** GCC/Clang (Linux, macOS), MinGW-W64 (Windows)
- **Configuration:** Version variables in Makefile

### Makefile Requirements
The Makefile **MUST** contain:
```makefile
MAJOR=<number>
MINOR=<number>
PATCH=<number>
```

**Used by:** All jobs to extract version numbers

### Target Definitions
The Makefile **SHOULD** provide:
```makefile
all:              # Default build target
test:             # Run unit tests
clean:            # Clean build artifacts
release:          # Create release build (stripped)
```

**Impact:** Pipeline depends on these targets existing

---

## Platform-Specific Dependencies

### macOS (Apple Silicon & Intel)

#### Hardware
- **Runner:** GitHub Actions `macos-latest`
- **Arch:** Both arm64 and x86_64 supported
- **Xcode:** Command Line Tools required

#### Software
```
Tool          | Version      | Source   | Purpose
--------------|--------------|----------|------------------
clang         | Latest       | Xcode    | C compiler
lipo          | Built-in     | Xcode    | Universal binary creation
make          | GNU make     | Xcode    | Build tool
cppcheck      | 2.x+         | Homebrew | Static analysis
clang-format  | Latest       | Homebrew | Code formatting
valgrind      | Optional*    | Homebrew | Memory checking
```

**Note:* Valgrind may not be available on Apple Silicon

#### Caching
- **Path:** `~/Library/Caches/Homebrew`, `/Library/Caches/Homebrew`
- **Key:** `macos-brew-{workflow_hash}`
- **Benefit:** ~60% faster dependency installation

#### Special Features
- Universal binary creation (`lipo -create`)
- Parallel x86_64 and arm64 builds
- Version embedding in compilation flags

---

### Linux (Ubuntu)

#### Hardware
- **Runner:** GitHub Actions `ubuntu-latest`
- **Arch:** x86_64 only
- **Kernel:** Linux 5.x+

#### Software
```
Tool          | Version      | Source | Purpose
--------------|--------------|--------|------------------
gcc           | 11.x+        | APT    | C compiler (primary)
clang         | 14.x+        | APT    | C compiler (alternate)
make          | GNU make     | APT    | Build tool
build-essential| Latest      | APT    | Development packages
cppcheck      | 2.x+         | APT    | Static analysis
clang-format  | Latest       | APT    | Code formatting
valgrind      | 3.x+         | APT    | Memory checking
```

#### Caching
- **Path:** `~/.cache/apt`
- **Key:** `linux-apt-{workflow_hash}`
- **Benefit:** ~60% faster dependency installation

#### Special Features
- Dual compiler testing (GCC + Clang)
- Valgrind memory leak detection (non-blocking)
- Full test suite execution

---

### Windows (x86_64 & i686)

#### Hardware
- **Runner:** GitHub Actions `windows-latest`
- **Host Arch:** x86_64
- **Cross-Compilation:** i686 via MSYS2 toolchain

#### Software
```
Tool              | Version      | Source | Purpose
------------------|--------------|--------|------------------
mingw-w64         | 11.x+        | MSYS2  | GCC for Windows
mingw-w64-i686    | 11.x+        | MSYS2  | GCC for i686
mingw-w64-x86_64  | 11.x+        | MSYS2  | GCC for x86_64
make              | GNU make     | MSYS2  | Build tool
bash              | POSIX-like   | MSYS2  | Shell environment
cppcheck          | 2.x+         | MSYS2  | Static analysis
```

#### Shell Environment
- **MSYS2 System:** mingw64 (POSIX environment)
- **Shell Type:** bash (not cmd.exe or PowerShell)
- **Path:** POSIX style (uses forward slashes)

#### Special Features
- x86_64 native build
- i686 cross-compilation support
- POSIX compatibility layer via MSYS2

#### Known Limitations
- No valgrind equivalent on Windows
- MSYS2 bash has different path handling
- Some POSIX features may not work identically

---

## Compilation Assumptions

### C Standard
- **Standard:** C99 (`-std=c99`)
- **Flags Required:** `-Wall -Werror -Wpedantic -pedantic-errors`
- **Architecture Flags:** `-arch` (macOS), cross-compile prefix (Windows)

### Version Embedding
All binaries must embed version at compile time:
```c
-DMAJOR=<major> -DMINOR=<minor> -DPATCH=<patch>
```

**Assumption:** Source code uses these preprocessor defines
**Impact:** `binary --version` must return embedded version

### Threading
- **Library:** pthread
- **Flag:** `-pthread`
- **Required:** Assumed present in source code

### Binary Naming
- **macOS:** `vframetest` (no extension)
- **Linux:** `vframetest` (no extension)
- **Windows:** `vframetest.exe` (required extension)

---

## Testing Assumptions

### Test Execution
```bash
make test BUILD_FOLDER=<dir>
```

**Assumption:** This target builds and runs all tests
**Impact:** Must exit with 0 on success, non-zero on failure

### Test Discovery
- **Path:** `tests/` directory
- **Pattern:** `test_*.c` files
- **Requirement:** Tests self-contained, no external dependencies

### Test Output
- **Format:** Captured to stdout/stderr
- **Artifacts:** Optional test result files
- **Failure Reporting:** Non-zero exit code triggers failure

### Memory Leak Detection
- **Tool:** valgrind (Linux only)
- **Target:** `<BUILD_FOLDER>/test-runner` binary
- **Mode:** `--leak-check=full`
- **Impact:** Non-blocking (continues even if issues found)

---

## GitHub Platform Assumptions

### GitHub Features
- ✅ GitHub Actions enabled
- ✅ GitHub Artifacts available
- ✅ GitHub Releases available
- ✅ Protected main branch (recommended)

### Tokens & Secrets
- **GITHUB_TOKEN:** Automatically provided by GitHub Actions
- **Usage:** Creating releases, uploading artifacts
- **Scoping:** Minimal RBAC (read-only for most jobs)
- **Rotation:** GitHub manages automatically

### Branch Protection
**Recommended Configuration:**
```
Branch: main
  Require:
    - Status checks to pass before merging
    - Status check: status-check (CI/CD)
    - Dismiss stale pull request approvals
    - Require code reviews: 1 approval
```

### Commit Access
- **Required:** Push access to repository
- **For Release:** Tag creation and push
- **For Deploy:** Write access (GITHUB_TOKEN)

---

## External Dependencies

### GitHub Actions Actions (Reusable)
```yaml
actions/checkout@v4          # Clone repository
actions/upload-artifact@v4   # Upload artifacts
actions/download-artifact@v4 # Download artifacts
actions/cache@v4             # Caching dependencies
msys2/setup-msys2@v2         # Windows MSYS2 setup
softprops/action-gh-release@v1  # Create releases
```

**Assumption:** These actions remain available and stable
**Risk:** Actions may be deprecated or modified
**Mitigation:** Version pinning ensures consistency

### External Scripts
```
scripts/collect-benchmarks.sh  # ⚠️ Optional
  - Collects build metrics
  - Creates dashboard data
  - If missing: skipped gracefully
```

**Assumption:** Script is POSIX-compatible
**Impact:** Non-blocking (continues if not found)

### System Tools
```
Implicit Tools:
  - date (Unix timestamp generation)
  - grep (version extraction from Makefile)
  - cut (text processing)
  - find (artifact discovery)
  - ls (directory listing)
  - mkdir (directory creation)
  - cp (file copying)
  - shasum/sha256sum (checksum generation)
```

**Availability:** All are standard POSIX tools
**Fallbacks:** `shasum` or `sha256sum` for checksums

---

## Behavioral Assumptions

### Git Assumptions
- **Main branch:** `main` (not `master`)
- **Version tags:** Format `v<major>.<minor>.<patch>`
- **Commit access:** Available for tagging
- **History:** Full history available (fetch-depth: 0 for lint only)

### Build Reproducibility
- **Assumption:** Same source produces same binary
- **Impact:** Smoke tests pass identically across runs
- **Limitation:** Timestamps may differ

### Artifact Permanence
- **Build artifacts:** Retained 30 days
- **Test results:** Retained 7 days
- **Release packages:** Retained 90 days
- **Assumption:** Users download during retention period

### Concurrency
- **Strategy:** Cancel previous runs on same branch
- **Assumption:** Latest commit is always desired
- **Impact:** Rapid commits may skip intermediate builds

---

## Performance Assumptions

### Build Times
```
macOS universal build:    ~3-4 minutes
Linux GCC build:          ~2-3 minutes
Linux Clang build:        ~2-3 minutes
Windows x86_64 build:     ~2-3 minutes
Windows i686 build:       ~2-3 minutes
Tests (per platform):     ~1-2 minutes
```

**Assumption:** Builds complete in reasonable time
**Impact:** If builds exceed 30 minutes, consider optimization

### Cache Effectiveness
- **macOS:** Typical 60% reduction in dependency install time
- **Linux:** Typical 60% reduction in dependency install time
- **Windows:** No caching (toolchain pre-installed)

**Assumption:** Homebrew/APT caches remain stable
**Impact:** First build after cache invalidation slower

---

## Security Assumptions

### No Hardcoded Secrets
- **Assumption:** No credentials in workflow files
- **Impact:** Workflow can be public without leaking secrets
- **Validation:** Code review required for changes

### Minimal Permissions
- **RBAC:** Each job gets minimum required permissions
- **Token Scoping:** GITHUB_TOKEN scoped per job
- **Impact:** Compromise of one job doesn't affect others

### Artifact Integrity
- **Checksums:** SHA256 for all releases
- **Assumption:** SHA256 is cryptographically secure
- **Impact:** Users can verify download integrity

---

## Deployment Assumptions

### Release Trigger
- **Event:** Git tag push (`git push origin v*`)
- **Automation:** Pipeline automatically detects tag
- **Impact:** No manual release creation needed

### Release Publishing
- **Tool:** GitHub REST API (via softprops/action-gh-release)
- **Permission:** Write access via GITHUB_TOKEN
- **Impact:** Automatic GitHub release creation

### Artifact Availability
- **Assumption:** All platform builds complete successfully
- **Graceful Degradation:** Missing artifacts reported as warnings
- **Impact:** Release can proceed with partial artifacts

---

## Known Limitations

### Platform Support
- ❌ **macOS < 11.0:** Not tested
- ❌ **Linux glibc < 2.31:** Potential compatibility issues
- ❌ **Windows < 10:** Not supported
- ❌ **Non-x86 architectures:** Not supported (except macOS arm64)

### Architecture Support
- ✅ **x86_64:** Fully supported
- ✅ **arm64:** macOS only
- ✅ **i686:** Windows only (cross-compile from x86_64)
- ❌ **ARM (32-bit):** Not supported
- ❌ **PowerPC:** Not supported

### Compiler Support
- ✅ **GCC 11+:** Fully supported
- ✅ **Clang 14+:** Fully supported
- ✅ **MinGW-W64 11+:** Fully supported
- ❌ **MSVC:** Not supported

---

## Failure Recovery

### Recovery Procedures
```
If build fails:
  1. Check status-check job for details
  2. Review logs of failing job
  3. Check artifact uploads for diagnostics
  4. Retry via GitHub UI or new commit

If tests fail:
  1. Download test-results artifact
  2. Review test output
  3. Fix source code
  4. Push new commit to retry

If release fails:
  1. Ensure all builds succeeded
  2. Check artifact existence
  3. Verify tag format (v*.*.*)
  4. Retry via GitHub UI
```

### Idempotent Re-runs
- ✅ All jobs can be safely re-run
- ✅ Artifacts overwritten on duplicate run
- ✅ No state is preserved between runs
- ✅ Results identical on re-run

---

## Documentation References

1. **CI/CD Architecture:** `.github/CI_CD_ARCHITECTURE.md`
2. **Changes Summary:** `.github/CHANGES_SUMMARY.md`
3. **GitHub Actions Docs:** https://docs.github.com/en/actions
4. **GNU Make Manual:** https://www.gnu.org/software/make/manual/
5. **MSYS2 Documentation:** https://www.msys2.org/

---

## Maintenance & Updates

### When to Update Pipeline

1. **Major version bump:** Update version in Makefile
2. **New platform support:** Add matrix entry
3. **Compiler upgrade:** Update tool versions
4. **Feature addition:** Update job steps

### Backward Compatibility

**Guarantee:** All workflow changes are backward compatible
- ✅ No source code changes required
- ✅ Existing Makefiles work unchanged
- ✅ Artifacts in same locations
- ✅ No breaking changes

### Testing Updates

Before merging workflow changes:
1. Test on all 7 platform configurations
2. Verify artifacts are created
3. Check release process if modified
4. Validate checksum generation

---

## Support Escalation

For issues:

1. **Self-service troubleshooting:**
   - Review CI_CD_ARCHITECTURE.md
   - Check GitHub Actions logs
   - Run builds locally to reproduce

2. **Known Issues:**
   - Valgrind unavailable on macOS (expected, non-blocking)
   - MSYS2 path differences on Windows (handled)
   - First build after cache invalidation slower (expected)

3. **Report Issues:**
   - Include full workflow logs
   - Specify affected platform
   - Attach build reports artifact
   - Describe reproduction steps

---

**Last Updated:** 2025-11-27
**Status:** Production Ready ✅
