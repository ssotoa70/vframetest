# Installation Guide

## macOS (Recommended: Homebrew)

### Using Homebrew
```bash
brew tap ssotoa70/vframetest https://github.com/ssotoa70/vframetest
brew install vframetest
vframetest --version
```

### Manual Installation (Pre-built Binary)
```bash
curl -L -O https://github.com/ssotoa70/vframetest/releases/download/v25.17.4/vframetest-25.17.4-macos-universal
chmod +x vframetest-25.17.4-macos-universal
mv vframetest-25.17.4-macos-universal /usr/local/bin/vframetest
vframetest --version
```

### Build from Source
```bash
git clone https://github.com/ssotoa70/vframetest.git
cd vframetest
make clean && make -j4
./build/vframetest --version
```

## Linux

### Pre-built Binary
```bash
wget https://github.com/ssotoa70/vframetest/releases/download/v25.17.4/vframetest-25.17.4-linux-x86_64
chmod +x vframetest-25.17.4-linux-x86_64
sudo mv vframetest-25.17.4-linux-x86_64 /usr/local/bin/vframetest
vframetest --version
```

### Build from Source
```bash
git clone https://github.com/ssotoa70/vframetest.git
cd vframetest
# Install dependencies (example for Ubuntu/Debian)
sudo apt-get install build-essential git

make clean && make -j4
sudo cp build/vframetest /usr/local/bin/
vframetest --version
```

## Windows

### Pre-built Executable
Download from [GitHub Releases](https://github.com/ssotoa70/vframetest/releases):
- `vframetest-25.17.4-windows-x86_64.exe` (64-bit)
- `vframetest-25.17.4-windows-i686.exe` (32-bit)

```powershell
# Download and place in system PATH, or run directly:
.\vframetest-25.17.4-windows-x86_64.exe --version
```

### Build from Source
```bash
# Requires MinGW and Git Bash or WSL
git clone https://github.com/ssotoa70/vframetest.git
cd vframetest
make win64   # or: make win (for 32-bit)
```

## Verification

After installation, verify the setup:
```bash
# Check version
vframetest --version

# Create test directory
mkdir -p /tmp/test-data

# Run basic test
vframetest -w FULLHD-24bit -t 2 -n 10 /tmp/test-data
```

Expected output should show:
- Profile: FULLHD-24bit
- Performance metrics (MiB/s, FPS)
- Frames failed: 0
- Frames succeeded: 10
- Success rate: 100.00%
- Filesystem: LOCAL

## Troubleshooting

### "Command not found"
Ensure `/usr/local/bin` is in your PATH, or use full path to binary.

### Permission Denied
```bash
chmod +x vframetest
```

### Build Failures
Ensure you have:
- C99-compatible compiler (gcc, clang)
- POSIX threads (pthread) support
- GNU Make

See [BUILD.md](../docs/BUILD.md) for detailed build instructions.

## Uninstallation

### macOS (Homebrew)
```bash
brew uninstall vframetest
brew untap ssotoa70/vframetest
```

### Manual
```bash
rm /usr/local/bin/vframetest
```

---

**Next**: See [USAGE.md](USAGE.md) for how to use vframetest
