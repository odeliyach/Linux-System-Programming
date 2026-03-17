# Interview Guide: Linux System Programming Portfolio

## The 30-Second Elevator Pitch

> "I built a minimal shell and thread-safe queue in pure C11 to demonstrate low-level systems expertise. The shell handles multi-stage pipelines with proper signal semantics and zombie prevention, while the queue achieves **~400K items/sec throughput** using per-thread condition variables instead of global broadcasts—a pattern I saw in production databases. Both components emphasize **correct resource management** over feature bloat, showing I can write maintainable systems code that scales."

**Key Hooks**:
- Performance metric (quantifiable impact)
- Production pattern reference (real-world connection)
- Trade-off awareness (engineering maturity)

---

## Top 5 Technical Questions (With Professional Answers)

### 1. **"Why did you use C11 threads instead of pthreads?"**

**Answer**:
"C11 threads (`threads.h`) provide a portable, standard API that works across POSIX and Windows with minimal code changes. For this portfolio piece, I wanted to show modern C proficiency—many developers default to pthreads without realizing C11 offers a cleaner interface. That said, in production at a company like Palo Alto Networks, I'd use pthreads for Linux-specific features like CPU affinity (`pthread_setaffinity_np()`) or real-time scheduling. The key is **choosing the right tool for the portability vs. performance trade-off**."

**Why This Works**:
- Shows you understand both APIs
- Demonstrates trade-off thinking (portability vs. features)
- Signals awareness of production environments

---

### 2. **"How do you prevent race conditions in your queue implementation?"**

**Answer**:
"The queue uses a single `mtx_t` to protect both the item queue and the wait queue—keeping the critical section simple reduces lock contention bugs. The key optimization is **per-thread condition variables**: when a consumer blocks, it gets a unique `cnd_t` instead of waiting on a shared one. This means producers can wake exactly one waiting consumer via `cnd_signal()`, avoiding the 'thundering herd' problem where dozens of threads wake up for one item.

I also use `atomic_fetch_add()` with `memory_order_relaxed` for the throughput counter—it's outside the mutex because reads don't need strict ordering, just eventual consistency for telemetry."

**Follow-Up Handling**:
- *"What if two producers enqueue simultaneously?"*
  → "The mutex serializes enqueue operations—only one producer can modify the queue head/tail at a time. The condition variable logic happens inside the lock, so handoffs are atomic."

- *"Could you use a lock-free queue instead?"*
  → "Yes, with CAS operations (`atomic_compare_exchange`), but you'd trade simplicity for complexity. For this demo, I wanted to show **correct concurrent programming first**—lock-free is an optimization you add once profiling proves the mutex is a bottleneck."

---

### 3. **"Explain your signal handling strategy in the shell."**

**Answer**:
"The shell needs to handle two signals carefully:

1. **SIGINT (Ctrl+C)**: The parent ignores it via `signal(SIGINT, SIG_IGN)`, but children inherit the default handler. This means Ctrl+C kills the foreground command without terminating the shell itself—standard shell semantics.

2. **SIGCHLD**: I install a handler that calls `waitpid(-1, &status, WNOHANG)` in a loop to reap any terminated child without blocking. This prevents zombie processes from accumulating.

The tricky part is **signal handler safety**: I save and restore `errno` because `waitpid()` can clobber it, and I use the `SA_RESTART` flag to automatically resume interrupted system calls like `read()` or `wait()`.

**Why WNOHANG?** Without it, `waitpid()` would block if a child is still running—but we want to reap only terminated children in the handler, not block the shell."

**Why This Works**:
- Shows deep understanding of signal semantics
- Explains the "why" behind every choice (interviewers love this)
- Anticipates edge cases (errno corruption, blocking behavior)

---

### 4. **"How would you debug a deadlock in your queue?"**

**Answer**:
"First, I'd use **GDB with thread debugging**:
```bash
gdb ./queue_test
(gdb) run
# Wait for hang
(gdb) info threads
(gdb) thread apply all bt  # Backtrace all threads
```
This shows where each thread is blocked—typically, you'll see multiple threads stuck in `mtx_lock()` or `cnd_wait()`.

Next, I'd check for **lock ordering violations**: if thread A locks mutex X then Y, but thread B locks Y then X, you get classic deadlock. My queue avoids this by only using **one mutex**, so lock ordering isn't an issue.

If the deadlock is in `cnd_wait()`, I'd look for **lost wakeups**: maybe a producer enqueued an item but called `cnd_signal()` *before* the consumer called `cnd_wait()`, causing the signal to be missed. The fix is ensuring the condition check happens inside the mutex:

```c
mtx_lock(&queue_lock);
while (!item_ready) {
    cnd_wait(&cond, &queue_lock);  // Atomically unlocks and waits
}
mtx_unlock(&queue_lock);
```

Finally, I'd use `valgrind --tool=helgrind` to detect data races—sometimes what looks like a deadlock is actually undefined behavior from missing synchronization."

**Why This Works**:
- Methodical debugging process (tool-based, not just guessing)
- Shows you've actually debugged threading issues before
- Demonstrates knowledge of tooling (GDB, Valgrind)

---

### 5. **"What's the biggest limitation of your shell implementation?"**

**Answer** (Honest + Forward-Looking):
"The shell is intentionally minimal—it doesn't support job control (no `fg`/`bg`), globbing (`*.txt`), or shell variables. The biggest **production gap** is **error handling for pipelines**: if a middle command fails (e.g., `cat file | nonexistent | wc`), the shell doesn't propagate the exit status clearly to the user.

In a real shell like bash, you'd use process groups (`setpgid()`) to manage pipeline jobs as a unit and track exit codes for each stage. I kept it simple here to focus on the **core mechanisms**—fork, exec, pipe, signal handling—without over-engineering.

If I were building this for production, I'd add:
1. **Robust error messages** (which command in the pipeline failed)
2. **Timeouts** for hung pipelines
3. **Resource limits** (max FDs, max processes) to prevent fork bombs

But for demonstrating systems knowledge, this shows I can **build the foundation** and understand what's missing."

**Why This Works**:
- Self-awareness (not pretending your code is perfect)
- Shows you know what production code looks like
- Demonstrates you can prioritize (foundation first, features second)

---

## Advanced Questions (For Senior Roles)

### Q: "How would you scale your queue to handle 1 million items/sec?"

**Answer**:
"At that scale, a single mutex becomes a bottleneck. I'd consider:

1. **Sharding**: Split into N independent queues, each with its own lock. Producers hash items to a shard (e.g., `item_id % N`), and consumers steal work from shards.

2. **Lock-free design**: Use a Michael-Scott queue (CAS-based) or LMAX Disruptor (ring buffer with sequence numbers). This eliminates mutex overhead but adds complexity—you need careful memory ordering and ABA problem handling.

3. **Batch operations**: Instead of enqueuing one item at a time, enqueue arrays of items. This amortizes lock acquisition costs.

4. **Profiling first**: I'd use `perf` to measure where time is spent—maybe the malloc in `enqueue()` is slower than the mutex. Switching to a memory pool might give more gains than lock-free algorithms.

**Key principle**: Don't optimize until you measure. My current design is correct and readable—those are prerequisites for performance tuning."

---

### Q: "Describe a time you debugged a memory leak in a production system."

**Pivot to Your Code**:
"While this portfolio project doesn't have leaks—I verified with Valgrind—here's my process when I encounter them in production:

1. **Reproduce locally**: Isolate the leak in a test harness
2. **Valgrind + massif**: Identify allocation sites and call stacks
3. **Ownership audit**: Trace where allocated memory should be freed
4. **Fix patterns**: Often it's missing `free()` in error paths or circular references in data structures

In my queue, every `malloc()` in `enqueue()` has a corresponding `free()` in `dequeue()`. The discipline is ensuring **clear ownership**: whoever allocates is responsible for deallocation, unless ownership is explicitly transferred."

---

## Behavioral Interviewing: Linking Code to Soft Skills

### Situation: "Tell me about a time you made a trade-off."

**Use Your Queue**:
"When designing the queue, I had to choose between a lock-free design (higher performance, more complex) and a mutex-based design (simpler, provably correct). I chose **simplicity** because:
- Lock-free bugs are notoriously hard to debug (ABA problem, memory ordering)
- The mutex version still achieves 400K items/sec, which is enough for most use cases
- A readable codebase makes it easier for teams to maintain

This reflects how I approach real projects: **optimize for maintainability unless performance is clearly the bottleneck**. You can always refactor later once you have benchmarks."

---

### Situation: "Describe a time you had to learn something quickly."

**Use Your Shell**:
"I learned C11 signal handling (`sigaction`, `SA_RESTART`) by reading POSIX docs and testing edge cases—like what happens if `waitpid()` is interrupted by another signal. I built a test harness that spawned 20 background processes and sent signals randomly to validate my handler logic.

This taught me that **systems programming requires reading specs carefully**—one wrong flag in `sigaction()` can cause race conditions that only appear under load."

---

## GitHub/Portfolio Talking Points

### When Showing Your Repo:

1. **Start with README performance table**
   → "Here's the throughput baseline—I include this so reviewers can quickly assess if the code matches their scale needs."

2. **Show the Makefile**
   → "I added `make check` for CI and `make debug` for Valgrind runs—production build systems need these knobs."

3. **Highlight TESTING.md**
   → "I documented the 3 highest-risk edge cases—race conditions, zombie processes, FD leaks. In an interview, I'd talk through how I'd test each one."

4. **Walk through `queue.c` structure**
   → "Notice the Doxygen comments on every function—I kept my original learning comments and added professional docstrings around them."

---

## Common Mistakes to Avoid

### ❌ Don't Say:
- "This is production-ready code." → **Too bold, invites skepticism**
- "I used pthreads because everyone does." → **Shows lack of critical thinking**
- "The queue is lock-free." → **If it's not, don't claim it**

### ✅ Do Say:
- "This demonstrates the fundamentals—here's what I'd add for production."
- "I chose C11 threads for portability, but I understand pthreads for Linux-specific features."
- "The queue uses mutexes for simplicity—I can explain when lock-free makes sense."

---

## Final Prep Checklist

**Before the Interview:**
- [ ] Run `make check` to ensure code compiles
- [ ] Test shell with: `ls | grep .c | wc -l`
- [ ] Run `valgrind --leak-check=full ./bin/queue_test`
- [ ] Review `SIGCHLD` handler logic (most common question)
- [ ] Prepare to explain one specific bug you fixed

**During Code Walkthrough:**
- [ ] Start with high-level architecture (shell vs. queue)
- [ ] Show performance metrics from README
- [ ] Explain one synchronization primitive in detail (mutex OR condition variable)
- [ ] Admit one limitation and how you'd fix it

**Closing Statement:**
"This project shows I can write systems code that's **correct, performant, and maintainable**. I focused on fundamentals—process management, thread synchronization, resource cleanup—because those are the skills that transfer across any systems role, whether it's kernel drivers, databases, or network stacks."

---

## Quick Reference: Buzzwords to Use Naturally

| Concept | Natural Phrase |
|---------|----------------|
| Thread safety | "I use a mutex to serialize access to shared state." |
| Performance | "The queue achieves 400K items/sec—enough for most OLTP workloads." |
| Resource management | "Every file descriptor opened in the child is closed before exec." |
| Signal handling | "I install a SIGCHLD handler with SA_RESTART to prevent zombie accumulation." |
| Trade-offs | "I chose simplicity over lock-free because the mutex version is easier to verify." |

---

**Remember**: Interviewers value **depth over breadth**. Master these 5 questions, and you'll stand out.
