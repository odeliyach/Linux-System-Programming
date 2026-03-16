# Contributing to Linux Systems Programming Portfolio

Thank you for your interest in contributing! This document provides guidelines for contributing to this project.

---

## 🎯 Project Goals

This is a **portfolio project** demonstrating production-grade systems programming practices. Contributions should maintain or improve:

1. **Code quality** - Clean, readable, well-documented C code
2. **Production standards** - Logging, error handling, testing
3. **Performance** - Maintain or improve benchmark results
4. **Security** - No vulnerabilities or unsafe practices

---

## 🚀 Getting Started

### Development Setup

```bash
# Clone the repository
git clone https://github.com/odeliyach/Linux-System-Programming.git
cd Linux-System-Programming

# Build with development flags
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_SANITIZERS=ON ..
cmake --build . --parallel $(nproc)

# Run tests
ctest --output-on-failure
```

### Required Tools

- **Compiler:** GCC 9+ or Clang 10+
- **Build System:** CMake 3.14+
- **Linting:** clang-format, clang-tidy
- **Testing:** CTest (included with CMake)
- **Optional:** Docker, valgrind

---

## 📝 Code Style

### C Coding Standards

We follow a strict subset of **MISRA C** and **CERT C** guidelines:

1. **Formatting:** Use `clang-format` (config in `.clang-format`)
   ```bash
   clang-format -i src/**/*.c include/**/*.h
   ```

2. **Naming Conventions:**
   - Functions: `camelCase` (e.g., `initQueue`, `processArglist`)
   - Variables: `snake_case` (e.g., `item_head`, `pipe_count`)
   - Constants/Macros: `UPPER_CASE` (e.g., `MAX_PIPE_CMDS`, `LOG_INFO`)
   - Types: `CamelCase` (e.g., `ItemNode`, `WaitNode`)

3. **Function Length:** Maximum 100 lines (prefer smaller, focused functions)

4. **Comments:**
   - Use Doxygen-style comments for functions
   - Inline comments only when logic is non-obvious
   - Explain "why" not "what"

5. **Error Handling:**
   - Always check return values of system calls
   - Use `LOG_ERROR` or `LOG_ERRNO` for errors
   - Never use `printf` for error reporting

### Example

```c
/**
 * @brief Enqueue an item in a FIFO manner.
 *
 * If consumers are waiting, hands off the item directly without
 * touching the queue. Otherwise, allocates a new node and appends.
 *
 * @param item Pointer to item to enqueue (must not be NULL)
 * @return 0 on success, -1 on allocation failure
 */
int enqueue(void *item)
{
    if (item == NULL) {
        LOG_ERROR("Cannot enqueue NULL item");
        return -1;
    }

    mtx_lock(&queue_lock);

    /* Check for waiting consumers (direct handoff optimization) */
    if (wait_head != NULL) {
        WaitNode *waiter = wait_head;
        wait_head = waiter->next;
        /* ... */
    }

    /* ... rest of implementation ... */
}
```

---

## 🧪 Testing Requirements

### Before Submitting a PR

1. **All tests must pass:**
   ```bash
   cd build && ctest --output-on-failure
   ```

2. **No sanitizer errors:**
   ```bash
   cmake -DENABLE_SANITIZERS=ON ..
   make && ctest
   ```

3. **Code coverage:** Aim for >80% for new code
   ```bash
   cmake -DENABLE_COVERAGE=ON ..
   make && ctest
   gcovr -r .. --html --html-details -o coverage.html
   ```

4. **Static analysis:** Zero clang-tidy warnings
   ```bash
   find src -name '*.c' | xargs clang-tidy -p build
   ```

### Writing Tests

- **Unit tests** for individual functions (`tests/queue/`, `tests/shell/`)
- **Integration tests** for component interactions (`tests/integration/`)
- **Stress tests** for concurrency edge cases (8+ threads, 1000+ items)

Example test structure:

```c
static int test_feature_name(void)
{
    LOG_INFO("Running test_feature_name");

    /* Setup */
    initQueue();

    /* Exercise */
    int result = enqueue(&test_data);

    /* Verify */
    assert(result == 0);
    assert(visited() == 0);  // Not dequeued yet

    /* Teardown */
    destroyQueue();

    LOG_INFO("test_feature_name: PASSED");
    return 0;  /* 0 = pass, non-zero = fail */
}
```

---

## 🔄 Pull Request Process

### 1. Fork and Branch

```bash
# Fork the repo on GitHub, then:
git clone https://github.com/YOUR_USERNAME/Linux-System-Programming.git
cd Linux-System-Programming
git checkout -b feature/your-feature-name
```

### 2. Make Changes

- Write clean, focused commits
- Follow the code style guide
- Add tests for new functionality
- Update documentation if needed

### 3. Test Locally

```bash
# Format code
find src include tests -name '*.c' -o -name '*.h' | xargs clang-format -i

# Run all checks
./scripts/run_all_checks.sh  # (if script exists, or run manually)
```

### 4. Submit PR

- Push to your fork: `git push origin feature/your-feature-name`
- Open a PR on GitHub
- Fill out the PR template completely
- Link any related issues

### 5. Review Process

- CI must pass (all platforms, sanitizers, etc.)
- Code review from maintainer
- Address feedback with new commits
- Once approved, maintainer will merge

---

## 🐛 Bug Reports

### Before Reporting

1. Check existing issues
2. Verify on latest `main` branch
3. Test with sanitizers enabled

### Bug Report Template

```markdown
**Environment:**
- OS: Ubuntu 22.04
- Compiler: GCC 11.3
- CMake: 3.22

**Steps to Reproduce:**
1. Build with `cmake -DCMAKE_BUILD_TYPE=Debug ..`
2. Run `./myshell`
3. Execute command: `cat file | grep foo`

**Expected Behavior:**
Pipeline should execute successfully.

**Actual Behavior:**
Segmentation fault on second command.

**Logs/Output:**
```
[2024-01-15 10:23:45] [ERROR] [shell_core.c:165] dup2 failed: Bad file descriptor (errno=9)
Segmentation fault (core dumped)
```

**Additional Context:**
Happens only when input file does not exist.
```

---

## 💡 Feature Requests

Before proposing a new feature:

1. Check if it aligns with project goals (portfolio demonstration)
2. Consider scope (this is not a production shell/queue library)
3. Discuss in an issue before implementing

---

## 🏆 Recognition

Contributors will be acknowledged in:
- `README.md` (Acknowledgments section)
- Git commit history
- GitHub contributors page

---

## 📚 Resources

- [The Linux Programming Interface](https://man7.org/tlpi/) - Michael Kerrisk
- [CERT C Coding Standard](https://wiki.sei.cmu.edu/confluence/display/c/SEI+CERT+C+Coding+Standard)
- [Doxygen Documentation](https://www.doxygen.nl/manual/)
- [CMake Best Practices](https://cmake.org/cmake/help/latest/guide/tutorial/index.html)

---

## 📧 Contact

For questions or discussions:
- **Open an issue** for bugs or feature requests
- **Email:** (see GitHub profile)
- **LinkedIn:** [Odeliya Chitayat](https://linkedin.com/in/odeliyach)

---

Thank you for contributing! 🙏
