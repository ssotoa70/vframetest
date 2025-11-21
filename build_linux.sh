#!/bin/bash
# Build vframetest for Linux (RHEL and Ubuntu/Debian)
# This script builds native binaries for Linux systems

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

echo -e "${GREEN}Building vframetest ${VERSION} for Linux${NC}"
echo ""

# Detect OS
if [ -f /etc/os-release ]; then
    . /etc/os-release
    OS_NAME=$NAME
    OS_ID=$ID
fi

# Build standard Linux binary
echo -e "${YELLOW}Building for Linux (generic x86_64)...${NC}"
make clean
CFLAGS="-std=c99 -O2 -Wall -Werror -Wpedantic -pedantic-errors -DMAJOR=${MAJOR} -DMINOR=${MINOR} -DPATCH=${PATCH}" \
LDFLAGS="-pthread" \
make

# Create build output directory
BUILD_DIR="build-linux-${OS_ID}"
mkdir -p "${BUILD_DIR}"

# Copy built binary
cp build/vframetest "${BUILD_DIR}/vframetest-${VERSION}-linux-x86_64"
cp build/libvframetest.a "${BUILD_DIR}/libvframetest.a"

echo ""
echo -e "${GREEN}✓ Linux build complete!${NC}"
echo "  Binary: ${BUILD_DIR}/vframetest-${VERSION}-linux-x86_64"
echo "  Library: ${BUILD_DIR}/libvframetest.a"
echo ""

# Optional: Build for ARM64 if compiler available
if command -v aarch64-linux-gnu-gcc &> /dev/null; then
    echo -e "${YELLOW}Building for Linux ARM64 (cross-compile)...${NC}"
    make clean
    CC=aarch64-linux-gnu-gcc AR=aarch64-linux-gnu-ar \
    CFLAGS="-std=c99 -O2 -Wall -Werror -Wpedantic -pedantic-errors -DMAJOR=${MAJOR} -DMINOR=${MINOR} -DPATCH=${PATCH}" \
    LDFLAGS="-pthread" \
    make

    cp build/vframetest "${BUILD_DIR}/vframetest-${VERSION}-linux-arm64"
    echo -e "${GREEN}✓ Linux ARM64 build complete!${NC}"
    echo "  Binary: ${BUILD_DIR}/vframetest-${VERSION}-linux-arm64"
fi

echo ""
echo -e "${GREEN}All Linux builds complete!${NC}"
