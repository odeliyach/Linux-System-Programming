# Interview Prep Guide (Word-Ready) – Linux System Programming Portfolio

Copy-paste this into a Word doc and keep the headings; each section states **what the code does**, **why it was built that way**, and **how to say it out loud**.

---

## 30-Second Pitch (say this first)
- **What**: “I built a minimal shell and a high-throughput thread-safe queue in C11. The shell wires pipelines with proper signal semantics; the queue hits ~400K items/sec using per-consumer condition variables.”
- **Why**: “I wanted to demonstrate production-grade systems fundamentals—process lifecycle, signals, and concurrency—without hiding behind frameworks.”
- **How to say it**: Confident, concise, end with a number (“~400K items/sec”) and a principle (“correctness before features”).

---

## Architecture Snapshot (for whiteboard)
- **Components**: `shell` (process/signal management), `queue` (concurrency), `tests` (throughput harness), `build/CI` (Makefile, Docker, GitHub Actions).
- **Flow**: Shell parses → forks children → pipes/dup2 wiring → signal handling prevents zombies. Queue serializes state with one mutex, parks consumers on per-thread `cnd_t`, producers hand off directly.
- **Why this split**: Keeps the repo teachable—two focused subsystems that surface classic OS/CS interview topics.

---

## File-by-File Talk Track
Use these bullets verbatim with recruiters. Each entry includes **What**, **Why**, **How to talk about it**.

### Top-Level
- **`README.md`**
  - *What*: Portfolio overview, quick start commands, performance table, and feature bullets.
  - *Why*: Gives recruiters an immediate “so what” (throughput metric + skills) before code dive.
  - *How*: “I front-loaded measurable impact and architecture so reviewers can qualify me in 30 seconds.”

- **`Makefile`**
  - *What*: Professional build targets (`all`, `test`, `check`, `debug`, `format`, `install`).
  - *Why*: Mirrors production CI knobs; enforces warnings-as-errors and consistent flags.
  - *How*: “I treat the build as a product—strict flags, reproducible targets, and a `check` target for CI.”

- **`Dockerfile`**
  - *What*: Two-stage build; compiles/tests in `gcc:11` then ships minimal `debian:bullseye-slim` with non-root user.
  - *Why*: Guarantees toolchain parity and small runtime surface; security via non-root execution.
  - *How*: “I bake tests into the image build so bad code can’t ship, and I drop privileges in the runtime stage.”

- **`.github/workflows/ci.yml`**
  - *What*: CI that installs toolchain, builds, runs tests, verifies binaries, and checks clang-format (dry-run).
  - *Why*: Proves reproducibility and style consistency on every push/PR.
  - *How*: “CI mirrors `make check` locally—no drift between my laptop and GitHub runners.”

### Shell
- **`src/shell/myshell.c`**
  - *What*: Core shell logic—fork/exec pipelines, `<`/`>` redirection, background support, SIGCHLD reaping, SIGINT delegation.
  - *Why*: Demonstrates process control and correct signal semantics; caps pipelines at 10 to keep FD management safe.
  - *How*: “Parent ignores SIGINT so children receive it; SIGCHLD handler loops with `waitpid(WNOHANG)` to avoid zombies; pipes are closed in every child to prevent FD leaks.”

- **`src/shell/shell_main.c`**
  - *What*: Minimal REPL: reads a line, tokenizes, calls `process_arglist`, frees resources.
  - *Why*: Separates parsing/driver from execution to keep the core shell testable and focused.
  - *How*: “I kept the driver tiny—parsing is explicit, memory ownership is clear, and all roads lead to the core process function.”

- **`docs/instructions_systems.txt`** (legacy assignment brief)
  - *What*: Original course assignment spec for the shell (signals, pipes, required behavior).
  - *Why*: Documents the constraints the implementation was built to satisfy.
  - *How*: “I preserved the assignment brief to show compliance context and to clarify intentionally omitted features like full job control.”

### Queue
- **`src/queue/queue.c`**
  - *What*: Thread-safe FIFO with one mutex, per-waiter condition variables, and relaxed atomic throughput counter.
  - *Why*: Single lock keeps correctness simple; per-waiter `cnd_t` avoids the thundering herd; relaxed atomics keep telemetry cheap.
  - *How*: “Consumers park with their own condition variable; producers hand off directly under the lock—O(1) wakeups, no broadcast storms.”

- **`include/queue.h`**
  - *What*: Public API (`initQueue`, `enqueue`, `dequeue`, `visited`, `destroyQueue`).
  - *Why*: Clean, minimal surface for reusability across tests/clients.
  - *How*: “Tiny API on purpose—easy to audit and drop into other C projects.”

### Tests & QA
- **`tests/queue_test.c`**
  - *What*: Producer/consumer harness (2P/2C) measuring throughput and verifying visited count.
  - *Why*: Exercises synchronization under concurrency and produces a brag-worthy metric.
  - *How*: “Workload is symmetric to expose race conditions; timing uses `CLOCK_MONOTONIC` to avoid wall-clock skew.”

- **`tests/TESTING.md`**
  - *What*: Edge-case catalog for queue and shell; includes Docker smoke tests and Valgrind guidance.
  - *Why*: Shows test philosophy and acknowledges untested gaps (e.g., destroy during use).
  - *How*: “I list what’s covered and what’s intentionally not—demonstrates prioritization and risk awareness.”

### Documentation
- **`docs/TECHNICAL_ANALYSIS.md`**
  - *What*: Q&A, talking points, and behavioral narratives for the project.
  - *Why*: Prepared answers improve interview confidence; links code to story.
  - *How*: “I pre-baked answers about signals, race conditions, and trade-offs so I can respond crisply.”

- **`docs/CV_FEEDBACK.md`**
  - *What*: Recruiter-style critique of the resume framing this project.
  - *Why*: Ensures language resonates with hiring signals (impact, metrics, clarity).
  - *How*: “I treat feedback as a spec—adjust wording to match what recruiters scan for.”

---

## Engineering Decisions (What/Why/How to Say)
- **Per-waiter condition variables (queue)**:  
  - *What*: Each waiting consumer has its own `cnd_t`; producers signal exactly one.  
  - *Why*: Avoids wake-all storms and context-switch waste; simpler than futexes/lock-free.  
  - *How*: “I traded theoretical peak throughput for predictable O(1) wakeups and simpler correctness.”

- **Single mutex for queue state**:  
  - *What*: One `mtx_t` guards both item and wait queues.  
  - *Why*: Eliminates lock-ordering risk; plenty fast for demonstrated load.  
  - *How*: “Correctness first—if profiling shows contention, I’d shard or go lock-free.”

- **SIGINT handling strategy (shell)**:  
  - *What*: Parent ignores SIGINT; children inherit default; background children can ignore when requested.  
  - *Why*: Matches common shell UX—Ctrl+C kills foreground job, not the shell.  
  - *How*: “I delegate SIGINT to children and restore defaults in `finalize()` to avoid surprising users.”

- **SIGCHLD reaping with `SA_RESTART`**:  
  - *What*: Handler loops `waitpid(-1, …, WNOHANG)` and preserves `errno`.  
  - *Why*: Prevents zombies without blocking; `SA_RESTART` reduces EINTR hassles.  
  - *How*: “I made the handler async-signal-safe and defensive—errno is restored to avoid corrupting callers.”

- **Pipeline cap at 10 commands**:  
  - *What*: Guardrail via `MAX_PIPE_CMDS`.  
  - *Why*: Avoids unbounded FD growth and simplifies static pipe array allocation.  
  - *How*: “Explicit limits show I think about resource ceilings; easy to raise with a ring buffer if needed.”

- **Docker multi-stage build**:  
  - *What*: Build/test in builder stage; slim runtime with non-root user.  
  - *Why*: Reproducible builds and least-privilege runtime.  
  - *How*: “Security and parity baked into the image; tests run before artifacts ship.”

- **CI formatting check as dry-run**:  
  - *What*: clang-format `--dry-run --Werror` to detect drift without rewriting.  
  - *Why*: Keeps PRs clean; avoids CI mutating code.  
  - *How*: “Style is enforced automatically, but humans stay in control of diffs.”

---

## CS Concepts (Define + Connect to Code)
- **Mutex (`mtx_t`)**: Mutual exclusion primitive; here, serializes queue state to prevent races.
- **Condition Variable (`cnd_t`)**: Wait/notify mechanism; used per-consumer to avoid broadcast storms.
- **Atomic Operations (`atomic_fetch_add`, relaxed order)**: Lock-free telemetry counter; correctness doesn’t depend on ordering, so relaxed is sufficient.
- **Producer-Consumer Pattern**: Coordinated work handoff; queue pairs producers and consumers via wait queue + item queue.
- **Fork/Exec Model**: `fork()` duplicates process; `execvp()` overlays child with target program—used per pipeline stage.
- **Pipes & `dup2`**: Anonymous pipes connect stdout of one process to stdin of the next; `dup2` wires FDs reliably.
- **Signals (SIGINT, SIGCHLD, `SA_RESTART`)**: Async notifications; shell ignores SIGINT in parent, reaps SIGCHLD to avoid zombies, relies on restart semantics to reduce EINTR.
- **Zombie Processes**: Children that ended but weren’t waited on; prevented by SIGCHLD handler.
- **Background vs. Foreground Jobs**: Background skips parent wait; foreground waits synchronously—mirrors shell UX.
- **Build Reproducibility**: Same flags and steps in Makefile, Docker, and CI ensure deterministic outputs.

---

## Metrics & Proof Points
- Queue throughput: **~400–465K items/sec** on GitHub runners (2 producers/2 consumers, 100 items each).
- Warnings-as-errors: `-Wall -Wextra -Werror`, `-O2`.
- Tests: `make test` (queue harness), `make check` (build + test), CI mirrors these.
- Security: Non-root container runtime; no `system()` calls—uses `execvp` directly.

---

## Quick Answers to Common Prompts
- **“Why not lock-free?”** → “Mutex version is simpler and already fast enough; I’d switch after profiling shows contention.”
- **“How do you prevent zombies?”** → “SIGCHLD handler loops `waitpid(WNOHANG)`, preserves errno, and uses `SA_NOCLDSTOP`.”
- **“How do you avoid lost wakeups?”** → “Condition wait/notify happens under the mutex; consumers enqueue their wait node before sleeping.”
- **“Biggest limitation?”** → “Shell lacks full job control and detailed pipeline exit codes; queue is unbounded and destroy isn’t safe under active load.”
- **“How would you scale the queue?”** → “Shard by hash, or adopt a Michael-Scott lock-free queue; optionally batch enqueues to amortize lock cost.”

---

## Walkthrough Script (3–5 minutes)
1. Start with the 30-second pitch (metrics + scope).
2. Open `README.md` to show performance table and architecture bullets.
3. Walk `queue.c`: one mutex, per-waiter `cnd_t`, relaxed atomic counter; mention tests and throughput.
4. Walk `myshell.c`: SIGINT strategy, SIGCHLD handler, pipe wiring, background guardrails.
5. Mention CI/Docker: tests run in build stage; non-root runtime.
6. Close with limitations + how you’d harden (job control, bounded queue, richer error reporting).

---

## Behavioral Hooks (tie code to soft skills)
- **Trade-off thinking**: “Chose simplicity over lock-free; added explicit limits; documented gaps.”
- **Ownership**: “Every allocation in enqueue is freed in dequeue; signal handlers restore errno.”
- **Testing mindset**: “Edge cases documented in `tests/TESTING.md`; CI + Docker ensures parity.”
- **Security awareness**: “Non-root container, no shell injection, explicit FD cleanup.”

---

## Last-Minute Checklist Before Interview
- Run `make check` locally (mirrors CI).
- Skim `myshell.c` signal blocks and `queue.c` wait-path to refresh wording.
- Keep the throughput number ready (~400–465K items/sec) and the O(1) wakeup story.
- Prepare one improvement you’d ship next (job control or bounded queue).

