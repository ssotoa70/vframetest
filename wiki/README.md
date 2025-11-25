# vframetest Wiki & Documentation

Welcome to the vframetest comprehensive documentation center!

## 📚 Wiki Pages

All documentation is available in this `/wiki` directory and can also be accessed through the GitHub web interface or repository documentation.

### Getting Started
- **[Home](Home.md)** - Welcome page with quick links and overview
- **[Features Guide](Features-Guide.md)** - Complete feature documentation with examples

### Understanding the Project
- **[Project Evolution](Project-Evolution.md)** - How vframetest evolved from the original Tuxera implementation to the modern fork
- **[Comprehensive Summary](Comprehensive-Summary.md)** - Master reference for the entire project
- **[Phase 1 Completion](Phase-1-Completion.md)** - Technical details of Phase 1 implementation

## 📖 Documentation Quick Links

### In Repository
| Document | Location | Purpose |
|----------|----------|---------|
| Project Evolution | `docs/PROJECT_EVOLUTION.md` | Complete project history |
| Features Guide | `docs/FEATURES_GUIDE.md` | All features with examples |
| Phase 1 Report | `PHASE_1_COMPLETION_REPORT.md` | Technical Phase 1 details |
| Release Summary | `RELEASE_SUMMARY.md` | Version comparison |
| Comprehensive Summary | `COMPREHENSIVE_PROJECT_SUMMARY.md` | Master overview |
| Build Instructions | `BUILD.md` | How to build from source |
| Deployment | `docs/DEPLOYMENT.md` | Production deployment |
| Runbooks | `docs/RUNBOOKS.md` | Operational procedures |

### On GitHub
| Resource | URL |
|----------|-----|
| Latest Release | https://github.com/ssotoa70/vframetest/releases/tag/v25.11.23 |
| GitHub Issues | https://github.com/ssotoa70/vframetest/issues |
| Bug Tracking | https://github.com/ssotoa70/vframetest/issues?labels=bug |
| Development Roadmap | https://github.com/ssotoa70/vframetest/issues?labels=enhancement |

## 🚀 Quick Start

### Installation
```bash
# macOS
brew install ssotoa70/vframetest/vframetest

# Linux/Windows
# Download from GitHub Releases
```

### Basic Test
```bash
# Single-thread test
vframetest -w FULLHD-24bit -t 1 -n 10 /mnt/storage

# Multi-thread test with all metrics
vframetest -w FULLHD-24bit -t 4 -n 100 --times /mnt/storage
```

## 📊 What's in Each Page

### Home.md
- Welcome and project overview
- Quick links to all resources
- Key features summary
- Latest version info

### Project-Evolution.md (800+ lines)
- Original Tuxera implementation history
- Feature evolution across versions
- Architecture improvements
- Code quality metrics
- User impact analysis
- Future roadmap

### Features-Guide.md (600+ lines)
- Complete feature documentation
- Real-world usage examples
- Platform-specific information
- Output format specifications
- Common use cases
- Troubleshooting guide

### Phase-1-Completion.md (300+ lines)
- Error tracking implementation details
- Filesystem detection technical specs
- Testing and validation results
- Known limitations
- Phase 2 roadmap

### Comprehensive-Summary.md (470+ lines)
- Master project overview
- All achievements summary
- Quality metrics
- Installation instructions
- Complete feature matrix
- Release timeline

## ✨ Key Features

### Phase 1 (v25.11.23)
- ✅ **Error Tracking** - errno capture, frame-level, thread identification
- ✅ **Filesystem Detection** - LOCAL/SMB/NFS with warnings
- ✅ **Success Metrics** - frames failed/succeeded/rate
- ✅ **Remote FS Warnings** - alerts for network storage limitations

### Infrastructure
- ✅ **Multi-Platform** - macOS, Linux, Windows
- ✅ **Universal Binaries** - Apple Silicon + Intel support
- ✅ **Automated CI/CD** - GitHub Actions for all platforms
- ✅ **Package Management** - Homebrew on macOS

## 🔍 Finding Information

**For beginners**: Start with [Home](Home.md) and [Features Guide](Features-Guide.md)

**For users**: Check [Features Guide](Features-Guide.md) for your use case

**For contributors**: Read [Project Evolution](Project-Evolution.md) to understand the codebase

**For administrators**: See [Comprehensive Summary](Comprehensive-Summary.md) for operations

**For technical details**: View [Phase 1 Completion](Phase-1-Completion.md)

## 🤝 Contributing

Contributions are welcome! See the [GitHub repository](https://github.com/ssotoa70/vframetest) for:
- Issue tracking
- Pull request guidelines
- Development roadmap

## 📞 Support

- **Issues & Bugs**: https://github.com/ssotoa70/vframetest/issues
- **Discussions**: GitHub Issues section
- **Documentation**: All available in this wiki

## 📈 Project Status

**Current Version**: 25.11.23
**Status**: Phase 1 Complete ✅ | Phase 2 In Progress 🚀
**Last Updated**: November 25, 2025
**Quality**: Production Ready | Enterprise Grade

---

**For the latest information, visit**: https://github.com/ssotoa70/vframetest
