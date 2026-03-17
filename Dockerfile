# ============================================================================
# Linux System Programming Portfolio - Production Container
# ============================================================================
# Multi-stage build for minimal production image
# ============================================================================

# Build stage
FROM gcc:11-bullseye AS builder

LABEL maintainer="System Programming Portfolio"
LABEL description="Minimal shell and thread-safe queue demonstration"

# Install build dependencies
RUN apt-get update && apt-get install -y --no-install-recommends \
    make \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /build

# Copy source files
COPY System_Programming_Projects/ System_Programming_Projects/
COPY src/ src/
COPY include/ include/
COPY tests/ tests/
COPY Makefile .

# Build the project (supports both legacy and new structure)
RUN make clean && make all

# Run tests during build to ensure correctness
RUN make test

# ============================================================================
# Runtime stage (minimal)
# ============================================================================
FROM debian:bullseye-slim

# Install minimal runtime dependencies
RUN apt-get update && apt-get install -y --no-install-recommends \
    libc6 \
    && rm -rf /var/lib/apt/lists/*

# Create non-root user for security
RUN useradd -m -s /bin/bash sysuser

WORKDIR /app

# Copy only the built binaries (handles both structures)
COPY --from=builder /build/bin/* /app/ 2>/dev/null || \
    (cp /build/myshell /app/ && cp /build/queue_test /app/)

# Set ownership
RUN chown -R sysuser:sysuser /app

# Switch to non-root user
USER sysuser

# Default command shows available binaries
CMD ["sh", "-c", "echo 'Available commands:' && ls -lh /app && echo '\nRun with: docker run -it <image> ./myshell'"]
