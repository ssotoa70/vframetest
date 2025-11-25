# Welcome to vframetest Wiki

vframetest is an enterprise-grade storage performance testing tool that evolved from the original Tuxera implementation.

## Quick Links

- **[Project Evolution](Project-Evolution)** - How vframetest evolved from the original implementation
- **[Features Guide](Features-Guide)** - Complete documentation of all features with examples
- **[Phase 1 Report](Phase-1-Completion)** - Error handling and filesystem detection implementation

## Latest Version

**v25.11.23** - Phase 1 Complete with comprehensive error tracking and filesystem detection

## Quick Start

```bash
# macOS
brew install ssotoa70/vframetest/vframetest

# Test storage performance
vframetest -w FULLHD-24bit -t 4 -n 100 /mnt/storage
```

## Key Features

✅ **Error Tracking** - Capture errno, frame-level failures, thread info
✅ **Filesystem Detection** - Identify LOCAL/SMB/NFS with warnings
✅ **Multi-Platform** - macOS, Linux, Windows support
✅ **Production Ready** - Enterprise-grade diagnostics

## More Information

- [GitHub Repository](https://github.com/ssotoa70/vframetest)
- [Latest Release](https://github.com/ssotoa70/vframetest/releases/tag/v25.11.23)
- [GitHub Issues](https://github.com/ssotoa70/vframetest/issues)
