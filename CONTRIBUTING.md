# Contributing to vframetest

Thank you for your interest in contributing to vframetest! This document provides guidelines and instructions for contributing to the project.

## Code of Conduct

Please read and follow our [Code of Conduct](CODE_OF_CONDUCT.md) to ensure a respectful and inclusive community.

## Getting Started

1. **Fork the repository** on GitHub
2. **Clone your fork** locally:
   ```bash
   git clone https://github.com/YOUR_USERNAME/vframetest.git
   cd vframetest
   ```
3. **Create a feature branch**:
   ```bash
   git checkout -b feature/your-feature-name
   ```

## Development Setup

### Build from Source

```bash
# macOS
make clean && make -j4

# Linux
make clean && make -j4

# Windows (with MinGW)
make clean && make -j4
```

### Testing

```bash
# Build the binary
make

# Run single-thread test
./build/vframetest -w FULLHD-24bit -t 1 -n 10 test-data

# Run multi-thread test
./build/vframetest -w FULLHD-24bit -t 4 -n 10 test-data

# Test all platforms (with CI container)
make test
```

## Making Changes

### Code Style

- Follow the existing code style in the repository
- Use `make` to build with `-Wall -Werror -Wpedantic` flags
- All code must compile without warnings
- Target C99 standard (`-std=c99`)

### Commit Messages

Follow conventional commits format:
- `feat:` for new features
- `fix:` for bug fixes
- `docs:` for documentation changes
- `refactor:` for code refactoring
- `test:` for test additions/changes
- `perf:` for performance improvements
- `ci:` for CI/CD changes

Example:
```
feat: add NFS performance optimization detection

- Detect nconnect parameter on NFS mounts
- Display optimization suggestions in output
- Add support for SMB multi-channel detection

Fixes #42
```

### Testing Your Changes

1. **Verify compilation**:
   ```bash
   make clean && make -j4
   ```

2. **Test basic functionality**:
   ```bash
   ./build/vframetest -w FULLHD-24bit -t 1 -n 5 test-data
   ```

3. **Test multi-threading**:
   ```bash
   ./build/vframetest -w FULLHD-24bit -t 4 -n 10 test-data
   ```

4. **Test different output formats**:
   ```bash
   ./build/vframetest -c -w FULLHD-24bit -t 1 -n 10 test-data  # CSV
   ./build/vframetest -j -w FULLHD-24bit -t 1 -n 10 test-data  # JSON
   ```

## Submitting Changes

1. **Push to your fork**:
   ```bash
   git push origin feature/your-feature-name
   ```

2. **Create a Pull Request** on GitHub with:
   - Clear title describing the change
   - Description of what changed and why
   - Reference to any related issues (e.g., "Fixes #42")
   - List of changes made

3. **PR Requirements**:
   - All tests must pass
   - Code must compile without warnings
   - Documentation updated if needed
   - Commit history should be clean (squash related commits)

## Pull Request Process

1. Your PR will be reviewed by maintainers
2. Address any feedback or requested changes
3. Once approved, your changes will be merged into main
4. Your contribution will be credited in the next release

## Reporting Bugs

Found a bug? Please create an issue on GitHub with:
- Clear title and description
- Steps to reproduce
- Expected vs actual behavior
- Your environment (OS, version, etc.)
- Any relevant output or error messages

## Suggesting Features

Have an idea? Create an issue with the "enhancement" label:
- Describe the feature and use case
- Explain why it would be valuable
- Suggest implementation approach if you have ideas

## Architecture and Code Organization

### Directory Structure

```
vframetest/
├── *.c, *.h          # Source files (core implementation)
├── Makefile          # Build system
├── .github/
│   └── workflows/    # CI/CD pipeline definitions
├── docs/             # Documentation
├── wiki/             # GitHub wiki pages
├── Formula/          # Homebrew formula
├── scripts/          # Build and utility scripts
├── tests/            # Test code
├── test-data/        # Test fixtures
└── dashboard/        # Benchmarking dashboard data
```

### Key Modules

- **frametest.c/h** - Main driver and test orchestration
- **platform.c/h** - Platform abstraction (macOS, Linux, Windows)
- **tester.c/h** - Test execution and result aggregation
- **report.c/h** - Output formatting (CSV, JSON)
- **histogram.c/h** - Performance visualization
- **profile.c/h** - Frame profile definitions
- **frame.c/h** - Frame data structures
- **timing.c/h** - High-resolution timing

## Performance Considerations

- Direct I/O is used when available for accurate measurements
- Thread-safe result aggregation for multi-threaded tests
- Memory-efficient frame buffer management
- Optimized platform-specific code paths

## Documentation

When adding features:
1. Update relevant documentation in `docs/` and `wiki/`
2. Add code comments for complex logic
3. Update the Features Guide if user-facing
4. Update release notes

## Questions?

- Check existing [issues](https://github.com/ssotoa70/vframetest/issues)
- Review [documentation](https://github.com/ssotoa70/vframetest/wiki)
- See [releases](https://github.com/ssotoa70/vframetest/releases) for examples

## License

By contributing to vframetest, you agree that your contributions will be licensed under the same GPL v2 license as the project.

---

Thank you for contributing to vframetest! 🎉
