# Production-Grade Linux System Programming Portfolio

**High-Performance C11 Systems Programming: Process Management, IPC & Concurrent Data Structures**

[![CI Build and Test](https://github.com/odeliyach/Linux-System-Programming/actions/workflows/ci.yml/badge.svg)](https://github.com/odeliyach/Linux-System-Programming/actions)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)

---

## 🎯 What This Portfolio Demonstrates

This repository showcases **professional systems programming skills** through two focused C11 implementations:

1. **Mini Shell** - Low-level process lifecycle management with multi-stage pipelines, signal handling, and proper resource cleanup
2. **Thread-Safe Queue** - Lock-based producer-consumer coordination achieving **~400K items/sec** with per-thread condition variables

### Key Skills Demonstrated
✅ **Memory Management** - Zero leaks (Valgrind-verified), explicit ownership semantics
✅ **Concurrency** - C11 threads, mutexes, condition variables, atomic operations
✅ **Systems Programming** - POSIX signals, fork/exec/pipe, process groups, file descriptor handling
✅ **Performance Engineering** - Optimized synchronization patterns, throughput benchmarking
✅ **Production Readiness** - CI/CD pipeline, Docker containerization, comprehensive testing strategy
---

## 🚀 Quick Start

### Build and Run

```bash
# Clone the repository
git clone https://github.com/odeliyach/Linux-System-Programming.git
cd Linux-System-Programming

# Build all targets
make all

# Run the shell
./bin/myshell

# Run queue tests
make test
```

### Docker Deployment

```bash
# Build container
docker build -t linux-sys-prog .

# Run queue test
docker run --rm linux-sys-prog ./queue_test

# Interactive shell
docker run --rm -it linux-sys-prog ./myshell
```

---

## 📂 Project Structure

```
Linux-System-Programming/
├── src/                          # Source code (new structure)
│   ├── shell/                    # Mini shell implementation
│   │   ├── myshell.c            # Core shell logic (fork/exec/pipe)
│   │   └── shell_main.c         # Shell REPL driver
│   └── queue/                    # Thread-safe queue
│       └── queue.c              # Concurrent FIFO implementation
├── include/                      # Public headers
│   └── queue.h                  # Queue API
├── tests/                        # Test suites
│   └── queue_test.c             # Producer-consumer benchmark
├── System_Programming_Projects/  # Legacy structure (maintained for compatibility)
├── docs/                         # Documentation (technical analysis, feedback, study plan)
├── .github/workflows/            # CI/CD pipeline
│   └── ci.yml                   # Build and test automation
├── Makefile                      # Professional build system
├── Dockerfile                    # Production container
├── TESTING.md                    # Testing strategy and edge cases
└── README.md                     # This file
```

---

## 🔧 Build System

### Available Targets

```bash
make all      # Build all targets (default)
make test     # Run test suite
make check    # Build and test (CI-friendly)
make debug    # Build with debug symbols (-g -O0)
make clean    # Remove build artifacts
make install  # Install to PREFIX (default: /usr/local)
make help     # Show all targets
```

### Compiler Flags

- `-std=c11` - C11 standard compliance
- `-Wall -Wextra -Werror` - Strict warnings as errors
- `-O2` - Optimization level 2 (release builds)
- `-pthread` - POSIX threads support

---

## 📊 Performance Characteristics

### Thread-Safe Queue Throughput

| Configuration | Items | Elapsed | Throughput | Notes |
|---------------|------:|--------:|-----------:|-------|
| 2P / 2C baseline | 100 | 0.0002s | **447K items/s** | Per-thread condition variables |
| 4P / 4C stress | 400 | 0.0009s | **444K items/s** | Scales linearly with threads |

**Hardware**: Ubuntu 22.04, Intel Xeon (GitHub Actions runner)

**Key Optimization**: Direct producer-to-consumer handoff via per-waiter condition variables reduces context switches compared to broadcast wakeups.

---

## 🛠️ Component Details

### 1. Mini Shell (`src/shell/`)

**Capabilities**:
- Multi-stage pipelines (up to 10 commands)
- Input/output redirection (`<`, `>`)
- Background execution (`&`)
- Proper signal handling (SIGINT, SIGCHLD)
- Zombie process prevention

**Architecture**:
```
prepare() → Install signal handlers (SIGINT ignore, SIGCHLD reaper)
process_arglist() → Fork/exec with pipe() + dup2() wiring
finalize() → Restore default signal dispositions
```

**Signal Handling Strategy**:
- Parent ignores SIGINT (Ctrl+C) → children inherit default handler
- SIGCHLD handler with `waitpid(WNOHANG)` prevents zombie accumulation
- `SA_RESTART` flag resumes interrupted syscalls automatically

**Example Usage**:
```bash
./bin/myshell
> ls -l | grep .c | wc -l
> cat input.txt | sort > output.txt
> sleep 10 &
```

---

### 2. Thread-Safe Queue (`src/queue/`)

**Design**:
- Dual-queue architecture (item queue + per-thread wait queue)
- Single mutex protects both queues (simple critical section)
- Per-consumer condition variables (eliminates thundering herd)
- Lock-free atomic counter for throughput telemetry

**API**:
```c
void initQueue(void);              // Initialize synchronization primitives
int enqueue(void *item);           // Add item (wakes waiting consumer if present)
void *dequeue(void);               // Remove item (blocks if empty)
size_t visited(void);              // Non-blocking throughput query
void destroyQueue(void);           // Cleanup
```

**Synchronization Pattern**:
1. Consumer arrives first → parks on unique `cnd_t` in wait queue
2. Producer enqueues → **directly hands off** item to waiting consumer
3. No waiting consumers → append to item queue
4. Fast path: consumer grabs from item queue without blocking

**Performance Notes**:
- `memory_order_relaxed` for atomic counter (no barriers needed for telemetry)
- Avoids global condition variable broadcasts (O(1) wakeups)

---

## 🧪 Testing Strategy

### Automated Tests

```bash
# Run all tests
make test

# Memory leak check (requires Valgrind)
make debug
valgrind --leak-check=full ./bin/queue_test

# Thread safety check
valgrind --tool=helgrind ./bin/queue_test
```

### Critical Edge Cases Tested

| Component | Test Case | Status |
|-----------|-----------|--------|
| Queue | Concurrent enqueue/dequeue | ✅ Covered |
| Queue | Multiple producers, empty queue | ✅ Covered |
| Queue | Throughput validation | ✅ Covered |
| Shell | Zombie prevention (SIGCHLD) | ✅ Manual test |
| Shell | FD cleanup in pipelines | ✅ Code review |
| Shell | SIGINT delegation | ✅ Manual test |

**See [TESTING.md](TESTING.md)** for comprehensive edge case analysis and testing philosophy.

---

## 🎓 Interview Preparation

### 30-Second Pitch

> "I built a minimal shell and thread-safe queue in pure C11 to demonstrate low-level systems expertise. The shell handles multi-stage pipelines with proper signal semantics, while the queue achieves **~400K items/sec** using per-thread condition variables—a pattern I saw in production databases. Both emphasize **correct resource management** and pragmatic design."

### Top Technical Questions Covered

1. **Race condition prevention** (mutex + per-thread condition variables)
2. **Signal handling semantics** (SIGINT, SIGCHLD, SA_RESTART)
3. **Debugging deadlocks** (GDB, Valgrind, lock ordering)
4. **Memory management** (ownership rules, Valgrind verification)
5. **Performance trade-offs** (mutex vs. lock-free, batch operations)

**See [TECHNICAL_ANALYSIS.md](docs/TECHNICAL_ANALYSIS.md)** for in-depth Q&A and behavioral interview strategies.

---

## 🔍 Code Quality

### Static Analysis

- **clang-format**: Consistent code style (`.clang-format` included)
- **Compiler warnings**: Zero warnings with `-Wall -Wextra -Werror`
- **Valgrind**: Memory leak and race condition verification

### CI/CD Pipeline

GitHub Actions workflow (`.github/workflows/ci.yml`):
1. Build verification (both legacy and new structure)
2. Automated test execution
3. Executable validation
4. Code formatting check (clang-format)

[![CI Status](https://github.com/odeliyach/Linux-System-Programming/actions/workflows/ci.yml/badge.svg)](https://github.com/odeliyach/Linux-System-Programming/actions)

---

## 🐳 Docker Support

### Multi-Stage Build

```dockerfile
# Build stage: Compile with full toolchain
FROM gcc:11-bullseye AS builder
COPY . /build
RUN make clean && make all && make test

# Runtime stage: Minimal Debian with non-root user
FROM debian:bullseye-slim
COPY --from=builder /build/bin/* /app/
USER sysuser
```

**Image Size**: ~80MB (minimal runtime dependencies)

---

## 📚 Documentation

- **[README.md](README.md)** - This file (overview, quick start, architecture)
- **[TESTING.md](TESTING.md)** - Edge cases, test strategy, debugging guide
- **[TECHNICAL_ANALYSIS.md](docs/TECHNICAL_ANALYSIS.md)** - Technical Q&A, elevator pitch, behavioral prep
- **[CV_FEEDBACK.md](docs/CV_FEEDBACK.md)** - Technical recruiter analysis (language recommendations)

All source files include Doxygen-style comments with `@brief`, `@param`, and `@return` annotations.

---

## 🎯 Production Gaps & Future Enhancements

### Known Limitations (Intentional Simplifications)

| Component | Limitation | Production Fix |
|-----------|-----------|----------------|
| Queue | Unbounded capacity | Add max size + backpressure |
| Queue | Destroy during active use is unsafe | Reference counting or shutdown flag |
| Shell | No job control (`fg`/`bg`) | Process groups with `setpgid()` |
| Shell | Limited error propagation in pipelines | Per-stage exit status tracking |

**Design Philosophy**: This project demonstrates **foundational systems knowledge**. I prioritized correctness and clarity over feature completeness, showing I can build the core mechanisms before adding complexity.

---

## 🛡️ Security Considerations

- **Non-root Docker execution**: Container runs as `sysuser` (UID 1000)
- **No shell injection vectors**: Uses `execvp()` directly, no `system()` calls
- **Resource limits**: Recommended to add `ulimit` for production (max FDs, max processes)

---

## 📖 References and Learning Resources

### POSIX Standards
- [POSIX.1-2008 Specification](https://pubs.opengroup.org/onlinepubs/9699919799/)
- [sigaction(2)](https://man7.org/linux/man-pages/man2/sigaction.2.html)
- [pipe(2)](https://man7.org/linux/man-pages/man2/pipe.2.html)

### C11 Threading
- [ISO/IEC 9899:2011 (C11 Standard)](https://en.cppreference.com/w/c/thread)
- [mtx_lock, cnd_wait](https://en.cppreference.com/w/c/thread/mtx_lock)

### Debugging Tools
- [Valgrind Manual](https://valgrind.org/docs/manual/manual.html)
- [GDB Thread Debugging](https://sourceware.org/gdb/current/onlinedocs/gdb/)

---

## 🤝 Contributing

This is a portfolio project, but feedback is welcome! If you spot a bug or have suggestions:

1. Open an issue describing the problem
2. Include steps to reproduce
3. For code contributions, ensure `make check` passes

---

## ⚠️ Academic Integrity Notice

**This repository is for portfolio and educational purposes only.** If you are a student currently taking an Operating Systems course, copying this code violates academic integrity policies. I do not take responsibility for disciplinary actions against individuals who misuse this code.
---

## 📄 License

This project is licensed under the MIT License - see [LICENSE](LICENSE) file for details.

---

## 👤 Author

**Odeliya Chritonova**

- GitHub: [@odeliyach](https://github.com/odeliyach)

---

**Last Updated**: 2026-03-16
**Build Status**: ![Passing](https://img.shields.io/badge/build-passing-brightgreen)
