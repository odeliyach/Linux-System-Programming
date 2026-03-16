# System Programming Lab Showcase

## **⚠️ Academic Integrity Warning**

**This repository is for portfolio and educational purposes only. If you are a student currently taking the OS course, copying this code violates academic integrity policies. I do not take responsibility for any disciplinary actions taken against individuals who misuse this code.**

---

This repository curates two focused systems programming exercises into a single lab:

- **Mini Shell** (`System_Programming_Projects/shell`): demonstrates process lifecycle management, pipelines, and basic redirection built on `fork`, `execvp`, and `pipe`.
- **Thread-Safe Queue** (`System_Programming_Projects/queue`): showcases producer-consumer coordination with C11 threads, mutexes, and condition variables.

## Project Layout

- `System_Programming_Projects/shell/`
  - `myshell.c`: core shell logic with signal handling and pipeline wiring.
  - `shell_main.c`: minimal driver to exercise the shell loop.
- `System_Programming_Projects/queue/`
  - `queue.c` / `queue.h`: blocking FIFO queue implementation.
  - `queue_test.c`: producer-consumer harness for quick validation and throughput sampling.
- `Makefile`: builds both artifacts; `make test` runs the queue harness.
- `.gitignore`: excludes build outputs and editor cruft.

## Build & Run

```bash
make            # builds myshell and queue_test
./myshell       # run the shell (supports &, |, <, > under the provided assumptions)
make test       # runs the queue producer/consumer harness
```

## Process Lifecycle & IPC

- `prepare()` installs signal handling so the shell ignores `SIGINT` while children use default semantics, and reaps exited children via a `SIGCHLD` handler to prevent zombies.
- `process_arglist()` uses `fork()` + `execvp()` to launch commands. For pipelines, it builds a chain of `pipe()` file descriptors and connects them with `dup2()` before `execvp()`, waiting for the full process group to finish. Background commands bypass the wait but still respect redirection assumptions.
- `finalize()` restores default dispositions before exit.

## Thread Synchronization (Producer-Consumer)

- A single mutex (`queue_lock`) protects both the ready queue and the wait queue.
- Consumers that arrive first park on a per-thread condition variable and receive items directly when a producer enqueues, reducing unnecessary context switches.
- `visited()` exposes a non-blocking, atomic counter for items that have traversed the queue, useful for lightweight telemetry while threads are active.

## Signal Handling

- The shell ignores `SIGINT` in the parent, delegates default handling to foreground children, and prevents zombie accumulation with a `SIGCHLD` handler using `waitpid(WNOHANG)`.
- Background processes are permitted for single commands; pipelines are intentionally foreground-only to keep process group control simple under the original assignment constraints.

## Performance Insights

| Scenario | Items | Elapsed (s) | Throughput (items/s) | Notes |
| --- | ---: | ---: | ---: | --- |
| Queue test harness (2 producers / 2 consumers) | 100 | 0.000311 | 321,261 | C11 threads, mutex + per-waiter condition variable |

Metrics come from the included `queue_test` run on this repository snapshot (`make test`). Use them as a baseline; rerun on your hardware for updated numbers.
