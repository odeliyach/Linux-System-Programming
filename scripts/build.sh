#!/bin/bash
# Build script for production-grade Linux systems programming project

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Configuration
BUILD_DIR="${BUILD_DIR:-build}"
BUILD_TYPE="${BUILD_TYPE:-Release}"
ENABLE_TESTS="${ENABLE_TESTS:-ON}"
ENABLE_BENCHMARKS="${ENABLE_BENCHMARKS:-ON}"
ENABLE_SANITIZERS="${ENABLE_SANITIZERS:-OFF}"
ENABLE_COVERAGE="${ENABLE_COVERAGE:-OFF}"
PARALLEL_JOBS="${PARALLEL_JOBS:-$(nproc)}"

echo -e "${GREEN}=== Linux Systems Programming Build Script ===${NC}"
echo "Build directory: $BUILD_DIR"
echo "Build type: $BUILD_TYPE"
echo "Tests: $ENABLE_TESTS"
echo "Benchmarks: $ENABLE_BENCHMARKS"
echo "Sanitizers: $ENABLE_SANITIZERS"
echo "Coverage: $ENABLE_COVERAGE"
echo "Parallel jobs: $PARALLEL_JOBS"
echo ""

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure
echo -e "${YELLOW}Configuring CMake...${NC}"
cmake -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
      -DBUILD_TESTS="$ENABLE_TESTS" \
      -DBUILD_BENCHMARKS="$ENABLE_BENCHMARKS" \
      -DENABLE_SANITIZERS="$ENABLE_SANITIZERS" \
      -DENABLE_COVERAGE="$ENABLE_COVERAGE" \
      ..

# Build
echo -e "${YELLOW}Building...${NC}"
cmake --build . --parallel "$PARALLEL_JOBS"

echo -e "${GREEN}Build complete!${NC}"
echo ""
echo "Executables:"
echo "  - myshell"
if [ "$ENABLE_TESTS" == "ON" ]; then
    echo "  - tests/*_test_runner"
fi
if [ "$ENABLE_BENCHMARKS" == "ON" ]; then
    echo "  - benchmarks/queue_benchmark"
fi
echo ""
echo "Run tests with: cd $BUILD_DIR && ctest --output-on-failure"
