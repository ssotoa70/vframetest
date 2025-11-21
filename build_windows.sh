#!/bin/bash
# Build vframetest for Windows (cross-compile)
# This script builds Windows binaries using MinGW cross-compiler

set -e

MAJOR=3025
MINOR=10
PATCH=2
VERSION="${MAJOR}.${MINOR}.${PATCH}"

# Color output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}Building vframetest ${VERSION} for Windows${NC}"
echo ""

# Check for MinGW toolchain
if ! command -v x86_64-w64-mingw32-gcc &> /dev/null; then
    echo -e "${RED}Error: x86_64-w64-mingw32-gcc not found!${NC}"
    echo "Install MinGW: brew install mingw-w64"
    exit 1
fi

if ! command -v i686-w64-mingw32-gcc &> /dev/null; then
    echo -e "${YELLOW}Warning: i686-w64-mingw32-gcc not found (32-bit builds disabled)${NC}"
fi

# Build Windows 64-bit
echo -e "${YELLOW}Building for Windows x86_64...${NC}"
BUILD_FOLDER="build-win64" CROSS=x86_64-w64-mingw32- ./build_win.sh "${VERSION}"
mkdir -p "build-windows"
cp "build-win64/vframetest.exe" "build-windows/vframetest-${VERSION}-windows-x86_64.exe"

echo ""
echo -e "${GREEN}✓ Windows x86_64 build complete!${NC}"
echo "  Binary: build-windows/vframetest-${VERSION}-windows-x86_64.exe"
echo ""

# Build Windows 32-bit if available
if command -v i686-w64-mingw32-gcc &> /dev/null; then
    echo -e "${YELLOW}Building for Windows i686...${NC}"
    BUILD_FOLDER="build-win32" CROSS=i686-w64-mingw32- ./build_win.sh "${VERSION}"
    cp "build-win32/vframetest.exe" "build-windows/vframetest-${VERSION}-windows-i686.exe"
    echo -e "${GREEN}✓ Windows i686 build complete!${NC}"
    echo "  Binary: build-windows/vframetest-${VERSION}-windows-i686.exe"
    echo ""
fi

echo -e "${GREEN}All Windows builds complete!${NC}"
