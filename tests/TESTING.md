# Testing Strategy and Edge Cases

## Overview

This document outlines the testing approach for the Linux System Programming portfolio project, focusing on **pragmatic, high-impact test cases** that demonstrate production-ready thinking.

---

## Test Infrastructure

### Current Setup
- **Build System**: GNU Make with dedicated `test` target
- **Test Framework**: Custom C benchmark harness (no external dependencies)
- **CI/CD**: GitHub Actions pipeline (build + test validation)
- **Code Quality**: clang-format for style consistency

### Running Tests

```bash
# Build and run all tests
make test

# Run with build verification
make check

# Legacy test mode
make legacy-test
```

---

## Thread-Safe Queue: Critical Edge Cases

### 1. **Race Condition: Multiple Producers, Empty Queue**

**Scenario**: Two producers enqueue simultaneously while multiple consumers wait.

**Expected Behavior**:
- Each waiting consumer should receive exactly one item
- No item should be delivered to multiple consumers
- No deadlock or lost wakeups

**Test Coverage**: ✓ Covered in `queue_test.c` with `NUM_PRODUCERS=2`, `NUM_CONSUMERS=2`

**Why It Matters**: This tests the core synchronization logic—incorrect mutex usage or condition variable handling causes data corruption or hangs.

---

### 2. **Producer-Consumer Starvation**

**Scenario**: High producer throughput with slow consumers.

**Expected Behavior**:
- Queue grows without bound (no backpressure in current design)
- No consumer thread should be starved
- All items must eventually be dequeued

**Current Limitation**: The queue has no max capacity—in production, this would need bounded queue semantics.

**Why It Matters**: Demonstrates understanding of real-world queue behavior and resource exhaustion risks.

---

### 3. **Destroy While Active**

**Scenario**: Call `destroyQueue()` while threads are still enqueuing/dequeuing.

**Expected Behavior**: **Undefined behavior** (not safe in current implementation)

**Production Fix Needed**:
- Add reference counting or shutdown flag
- Block `destroyQueue()` until all threads exit
- Wakeup all waiting consumers before destroying condition variables

**Why It Matters**: Exposes lifecycle management gaps—a common interview question.

---

### Edge Case Testing Recommendations

| Test Case | Priority | Current Status | Interview Value |
|-----------|----------|----------------|-----------------|
| Concurrent enqueue/dequeue | High | ✓ Covered | Core competency |
| Empty queue dequeue (blocking) | High | ✓ Covered | Synchronization understanding |
| Rapid destroy after init | Medium | ⚠ Not tested | Lifecycle awareness |
| Single producer, many consumers | Low | ⚠ Not tested | Fairness/starvation knowledge |
| Large item count (stress test) | Medium | ⚠ Configurable | Performance scaling |

---

## Mini Shell: Critical Edge Cases

### 1. **Zombie Process Prevention**

**Scenario**: Background process terminates while shell is busy.

**Expected Behavior**:
- `SIGCHLD` handler asynchronously reaps children
- No zombie processes accumulate (`ps aux | grep defunct`)

**Test Method**:
```bash
./myshell
> sleep 1 &
> sleep 2 &
> ps -o stat,pid,cmd | grep sleep
```

**Why It Matters**: Classic systems programming pitfall—forgetting to handle `SIGCHLD` causes resource leaks.

---

### 2. **Pipeline Error Propagation**

**Scenario**: Middle command in pipeline fails.

**Example**:
```bash
cat file.txt | nonexistent_command | wc -l
```

**Expected Behavior**:
- Shell should handle `execvp()` failure in child process
- Avoid broken pipe errors cascading incorrectly

**Current Limitation**: Error handling could be more explicit (e.g., logging which command failed).

**Why It Matters**: Shows understanding of process exit codes and pipeline semantics.

---

### 3. **File Descriptor Leak in Pipelines**

**Scenario**: Create a 10-stage pipeline.

**Expected Behavior**:
- All intermediate pipe FDs must be closed in child processes
- Check with `lsof -p <shell_pid>` after pipeline completes

**Current Status**: ✓ Implementation uses `close_pipe_set()` to clean up

**Why It Matters**: FD exhaustion is a common bug in shell implementations.

---

### Shell Edge Case Testing Recommendations

| Test Case | Priority | Current Status | Interview Value |
|-----------|----------|----------------|-----------------|
| Zombie reaping (`&` commands) | High | ✓ Handled | Core systems knowledge |
| FD cleanup in pipelines | High | ✓ Handled | Resource management |
| SIGINT during foreground cmd | Medium | ✓ Handled | Signal semantics |
| Redirection with missing files | Low | ⚠ Not tested | Error handling |
| Max pipeline depth (10 cmds) | Low | ⚠ Not tested | Edge case thinking |

---

## Integration Testing (Docker)

### Smoke Test in Container

```bash
# Build container
docker build -t linux-sys-prog .

# Run queue test inside container
docker run --rm linux-sys-prog ./queue_test

# Interactive shell test
docker run --rm -it linux-sys-prog ./myshell
```

**What This Proves**:
- Reproducible build environment
- No hidden host dependencies
- Production-grade deployment thinking

---

## Performance Benchmarking

### Queue Throughput Test

**Baseline**: ~320,000-450,000 items/sec (2 producers/2 consumers, 100 items)

**Metrics to Track**:
- Items processed per second
- Elapsed time variance across runs
- Memory usage (via `valgrind --tool=massif`)

**Scaling Test** (modify `queue_test.c`):
```c
#define NUM_PRODUCERS 4
#define NUM_CONSUMERS 4
#define ITEMS_PER_PRODUCER 1000
```

**Expected Interview Question**: "How would you optimize this for 100 producers?"
- **Answer**: Consider lock-free queue (CAS operations), work-stealing, or sharding

---

## Memory Safety Validation

### Valgrind Check

```bash
# Build with debug symbols
make debug

# Check for leaks
valgrind --leak-check=full ./bin/queue_test

# Check for race conditions
valgrind --tool=helgrind ./bin/queue_test
```

**Expected Result**: Zero leaks, no race condition warnings (with correct synchronization)

---

## Test Coverage Philosophy

### What We Test
✓ **Core synchronization logic** (mutex, condition variables)
✓ **Process lifecycle management** (fork, exec, wait, signals)
✓ **Resource cleanup** (FDs, memory, threads)
✓ **Throughput validation** (performance baseline)

### What We Don't Test (Pragmatic Trade-offs)
✗ GUI or complex input parsing (not the focus)
✗ Every possible shell command combination (assignment constraints)
✗ Network or distributed scenarios (out of scope)

---
## Running the Full Test Suite

```bash
# 1. Clean build and test
make clean && make check

# 2. Verify in Docker
docker build -t test . && docker run --rm test ./bin/queue_test

# 3. Memory check (requires valgrind)
make debug
valgrind --leak-check=full --show-leak-kinds=all ./bin/queue_test
```

**Total Time**: ~30 seconds for full validation cycle
