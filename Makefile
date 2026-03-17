# ============================================================================
# Linux System Programming Portfolio - Professional Build System
# ============================================================================

# Compiler and flags
CC       := gcc
CFLAGS   := -std=c11 -Wall -Wextra -Werror -O2
DEBUGFLAGS := -g -O0 -DDEBUG
THREADS  := -pthread
LDFLAGS  :=

# Directories
SRC_DIR    := src
INC_DIR    := include
TEST_DIR   := tests
BUILD_DIR  := build
BIN_DIR    := bin

# Source files
SHELL_SRCS  := $(SRC_DIR)/shell/myshell.c $(SRC_DIR)/shell/shell_main.c
QUEUE_SRCS  := $(SRC_DIR)/queue/queue.c
QUEUE_TEST  := $(TEST_DIR)/queue_test.c

# Build targets
MYSHELL     := $(BIN_DIR)/myshell
QUEUE_TEST_BIN := $(BIN_DIR)/queue_test

# ============================================================================
# Phony targets
# ============================================================================
.PHONY: all clean test check debug help install format

# Default target
all: $(BIN_DIR) $(MYSHELL) $(QUEUE_TEST_BIN)

# ============================================================================
# Build rules
# ============================================================================

# Create build directories
$(BIN_DIR):
	@mkdir -p $(BIN_DIR)

# Build shell
$(MYSHELL): $(SHELL_SRCS) | $(BIN_DIR)
	@echo "Building myshell..."
	$(CC) $(CFLAGS) -I$(INC_DIR) -o $@ $^ $(LDFLAGS)
	@echo "✓ myshell built successfully"

# Build queue test
$(QUEUE_TEST_BIN): $(QUEUE_SRCS) $(QUEUE_TEST) | $(BIN_DIR)
	@echo "Building queue_test..."
	$(CC) $(CFLAGS) $(THREADS) -I$(INC_DIR) -o $@ $^ $(LDFLAGS)
	@echo "✓ queue_test built successfully"

# ============================================================================
# Test and validation
# ============================================================================

# Run all tests
test: $(QUEUE_TEST_BIN)
	@echo "Running queue producer-consumer test..."
	@$(QUEUE_TEST_BIN)
	@echo "✓ All tests passed"

# Code quality check (runs tests and validates build)
check: all test
	@echo "✓ All checks passed"

# ============================================================================
# Debug builds
# ============================================================================

debug: CFLAGS := -std=c11 -Wall -Wextra $(DEBUGFLAGS)
debug: clean all
	@echo "✓ Debug build complete"

# ============================================================================
# Code formatting
# ============================================================================

# Format code using clang-format (if available)
format:
	@if command -v clang-format >/dev/null 2>&1; then \
		echo "Formatting source files..."; \
		find $(SRC_DIR) $(INC_DIR) $(TEST_DIR) -name "*.c" -o -name "*.h" | xargs clang-format -i; \
		echo "✓ Code formatted"; \
	else \
		echo "clang-format not found. Install with: sudo apt-get install clang-format"; \
	fi

# ============================================================================
# Cleanup
# ============================================================================

clean:
	@echo "Cleaning build artifacts..."
	@rm -rf $(BIN_DIR) $(BUILD_DIR)
	@rm -f *.o
	@echo "✓ Clean complete"

# ============================================================================
# Installation (local)
# ============================================================================

PREFIX ?= /usr/local
install: all
	@echo "Installing to $(PREFIX)/bin..."
	@install -d $(PREFIX)/bin
	@install -m 755 $(MYSHELL) $(PREFIX)/bin/
	@echo "✓ Installation complete"

# ============================================================================
# Help
# ============================================================================

help:
	@echo "Linux System Programming Portfolio - Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all          - Build all targets (default)"
	@echo "  test         - Run test suite"
	@echo "  check        - Build and test (CI-friendly)"
	@echo "  debug        - Build with debug symbols"
	@echo "  format       - Format code with clang-format"
	@echo "  clean        - Remove build artifacts"
	@echo "  install      - Install to PREFIX (default: /usr/local)"
	@echo "  help         - Show this help message"
	@echo ""
	@echo "Examples:"
	@echo "  make              # Build all"
	@echo "  make test         # Run tests"
	@echo "  make check        # Build and test"
	@echo "  make debug        # Debug build"
	@echo "  make PREFIX=~/.local install  # Install locally"
