#!/bin/bash
# Build vframetest for all supported platforms
# Builds: macOS (universal), Linux (RHEL/Ubuntu), Windows (MinGW)

set -e

MAJOR=3025
MINOR=10
PATCH=2
VERSION="${MAJOR}.${MINOR}.${PATCH}"

# Color output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Create output directory
OUTPUT_DIR="releases/vframetest-${VERSION}"
mkdir -p "${OUTPUT_DIR}"

echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}vframetest ${VERSION} - Multi-Platform Build${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
echo ""

# Function to build macOS
build_macos() {
    echo -e "${YELLOW}[1/3] Building for macOS...${NC}"

    # ARM64 build
    make clean
    make
    cp build/vframetest "${OUTPUT_DIR}/vframetest-${VERSION}-macos-arm64"

    # x86_64 build
    make clean
    CFLAGS="-arch x86_64 -std=c99 -O2 -Wall -Werror -Wpedantic -pedantic-errors -DMAJOR=${MAJOR} -DMINOR=${MINOR} -DPATCH=${PATCH}" \
    LDFLAGS="-arch x86_64 -pthread" \
    make
    cp build/vframetest "${OUTPUT_DIR}/vframetest-${VERSION}-macos-x86_64"

    # Universal binary
    lipo -create \
        "${OUTPUT_DIR}/vframetest-${VERSION}-macos-arm64" \
        "${OUTPUT_DIR}/vframetest-${VERSION}-macos-x86_64" \
        -output "${OUTPUT_DIR}/vframetest-${VERSION}-macos-universal"

    echo -e "${GREEN}✓ macOS builds complete${NC}"
    echo "  - vframetest-${VERSION}-macos-arm64 (Apple Silicon)"
    echo "  - vframetest-${VERSION}-macos-x86_64 (Intel)"
    echo "  - vframetest-${VERSION}-macos-universal (Both architectures)"
    echo ""
}

# Function to build Linux
build_linux() {
    echo -e "${YELLOW}[2/3] Building for Linux...${NC}"

    # Check if we're on Linux
    if [[ "$OSTYPE" == "linux-gnu"* ]]; then
        make clean
        CFLAGS="-std=c99 -O2 -Wall -Werror -Wpedantic -pedantic-errors -DMAJOR=${MAJOR} -DMINOR=${MINOR} -DPATCH=${PATCH}" \
        LDFLAGS="-pthread" \
        make
        cp build/vframetest "${OUTPUT_DIR}/vframetest-${VERSION}-linux-x86_64"
        echo -e "${GREEN}✓ Linux x86_64 build complete${NC}"
        echo "  - vframetest-${VERSION}-linux-x86_64"
    else
        echo -e "${YELLOW}⊘ Linux builds skipped (not on Linux)${NC}"
        echo "  Run on Linux with: ./build_linux.sh"
    fi
    echo ""
}

# Function to build Windows
build_windows() {
    echo -e "${YELLOW}[3/3] Building for Windows...${NC}"

    # Check for MinGW
    if command -v x86_64-w64-mingw32-gcc &> /dev/null; then
        BUILD_FOLDER="build-win64" CROSS=x86_64-w64-mingw32- ./build_win.sh "${VERSION}"
        cp "build-win64/vframetest.exe" "${OUTPUT_DIR}/vframetest-${VERSION}-windows-x86_64.exe"
        echo -e "${GREEN}✓ Windows x86_64 build complete${NC}"
        echo "  - vframetest-${VERSION}-windows-x86_64.exe"
    else
        echo -e "${YELLOW}⊘ Windows builds skipped (MinGW not installed)${NC}"
        echo "  Install with: brew install mingw-w64"
        echo "  Run with: ./build_windows.sh"
    fi
    echo ""
}

# Detect platform and build accordingly
if [[ "$OSTYPE" == "darwin"* ]]; then
    build_macos
    echo -e "${YELLOW}For Linux and Windows builds:${NC}"
    echo "  - ./build_linux.sh  (run on Linux)"
    echo "  - ./build_windows.sh (requires MinGW toolchain)"
elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
    build_linux
    echo -e "${YELLOW}Note: For macOS builds, run on macOS:${NC}"
    echo "  - ./build_all.sh"
else
    echo -e "${RED}Unsupported OS: $OSTYPE${NC}"
    exit 1
fi

echo ""
echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
echo -e "${GREEN}Build summary:${NC}"
echo ""
ls -lh "${OUTPUT_DIR}/"
echo ""
echo -e "${GREEN}All builds available in: ${OUTPUT_DIR}/${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
