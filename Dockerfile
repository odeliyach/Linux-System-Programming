# Multi-stage Dockerfile for production-grade build and deployment
# Stage 1: Build environment
FROM ubuntu:22.04 AS builder

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    clang \
    clang-format \
    clang-tidy \
    git \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /build

# Copy source code
COPY . .

# Build with CMake
RUN mkdir -p build && cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release \
          -DBUILD_TESTS=ON \
          -DBUILD_BENCHMARKS=ON \
          .. && \
    cmake --build . --parallel $(nproc)

# Run tests
RUN cd build && ctest --output-on-failure

# Stage 2: Runtime environment (minimal)
FROM ubuntu:22.04 AS runtime

# Install runtime dependencies only
RUN apt-get update && apt-get install -y \
    libpthread-stubs0-dev \
    && rm -rf /var/lib/apt/lists/*

# Create non-root user for security
RUN useradd -m -u 1000 sysuser

# Copy built binaries from builder
COPY --from=builder /build/build/myshell /usr/local/bin/
COPY --from=builder /build/build/libqueue_lib.a /usr/local/lib/
COPY --from=builder /build/build/libshell_lib.a /usr/local/lib/
COPY --from=builder /build/build/libcommon_utils.a /usr/local/lib/

# Set permissions
RUN chown -R sysuser:sysuser /usr/local/bin/myshell

# Switch to non-root user
USER sysuser
WORKDIR /home/sysuser

# Default command
CMD ["/usr/local/bin/myshell"]

# Metadata
LABEL maintainer="Odeliya Chitayat"
LABEL description="Production-grade Linux system programming demonstration"
LABEL version="1.0.0"
