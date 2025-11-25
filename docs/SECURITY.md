# Security Considerations

## Code Security

### Compilation Security
vframetest is compiled with strict security flags:
- `-Wall -Werror -Wpedantic` - All warnings as errors
- `-std=c99` - C99 standard compliance
- ASLR/PIE support on all platforms

### Memory Safety
- Manual memory management reviewed for leaks
- Buffer bounds checking
- Null pointer checks in critical paths
- No unsafe functions (strcpy, sprintf, gets)

### Error Handling
- Comprehensive error checking on all system calls
- Graceful error recovery
- No undefined behavior on malformed input

## Reporting Security Issues

Please do NOT open public issues for security vulnerabilities.

Contact: ssotoa70@gmail.com with:
- Detailed vulnerability description
- Steps to reproduce
- Potential impact assessment
- Suggested fix (optional)

## Dependencies

vframetest has **zero external dependencies** beyond:
- C standard library
- POSIX threads (system-provided)
- Platform APIs (macOS/Linux/Windows native)

## Input Validation

### Command-Line Arguments
- Directory path validation
- Integer range checking
- Profile name validation
- Thread count bounds checking

### File Operations
- Path traversal checks
- Write permission validation
- Disk space checking
- File handle limits

## Data Handling

### Test Data
- Temporary test files in specified directory only
- Automatic cleanup on completion
- No data persistence (unless explicitly exported)
- CSV/JSON export explicitly requested

### Performance Data
- No personally identifiable information (PII) collected
- No telemetry or phone-home functionality
- All data processing is local

## Platform-Specific Security

### macOS
- Code signing compatible
- Notarization compatible
- No privileges required
- Secure filesystem paths

### Linux
- POSIX permission compliance
- No sudo required for normal operation
- SELinux compatible
- AppArmor compatible

### Windows
- Admin privileges not required
- NTFS security attributes respected
- No registry modifications
- UAC-aware

## Best Practices for Users

1. **Run with appropriate permissions**
   - Don't run as root/admin unless necessary
   - Test directories should have appropriate permissions

2. **Test directory isolation**
   - Use dedicated test directories
   - Don't run tests on production data
   - Ensure adequate disk space

3. **Network storage caution**
   - Network shares may have different security implications
   - Test with non-sensitive data on network storage
   - Be aware of network monitoring on shared systems

4. **Access control**
   - Test directories should have restricted access
   - Especially for performance data on shared systems
   - CSV/JSON exports may contain system information

## Vulnerability Disclosure

vframetest follows coordinated vulnerability disclosure:
1. Report to maintainers privately
2. Allow time for patch development (usually 30 days)
3. Public disclosure after patch release
4. Credit to reporter (with permission)

## Updates & Patches

- Security updates released promptly
- All fixes backported to supported versions
- Release notes clearly indicate security fixes
- GitHub security advisories used for notification

## Compliance

vframetest complies with:
- GPL v2 license requirements
- POSIX standards
- Platform security guidelines
- Community coding standards

---

**Questions?** Contact ssotoa70@gmail.com
