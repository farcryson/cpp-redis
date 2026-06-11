# Mini-Redis (High-Performance Key-Value Store)

### Tech Stack
**C++** | **poll() (I/O Multiplexing)** | **POSIX Sockets**

---

### Key Implementation Details

* **Event-Driven Architecture:** Instead of creating a thread per client (which consumes RAM and CPU), this server uses `poll()` to monitor hundreds of file descriptors simultaneously on a single thread.
* **Pipelining Support:** The command parser (`process_commands`) is designed to handle multiple commands in a single TCP packet. This allows the server to hit **606,000 RPS** by reducing the number of `read()`/`send()` syscalls.
* **Memory Efficiency:** Zero locking overhead (**mutex-free**) because the single-threaded design guarantees data consistency by definition.

---

### Benchmark Results
*Tested on WSL (Windows Subsystem for Linux)*



```bash
redis-benchmark -p 6379 -t set,get -q -P 10
```
**SET:** 606,060.56 requests per second **GET:** 602,409.69 requests per second

**Note on WSL Performance:** The above results represent peak observed performance. Because WSL dynamically shares resources (CPU scheduling, RAM, network I/O) with the Windows host, day-to-day benchmark results can fluctuate significantly based on background OS processes or thermal limits. The heavy lifting here is achieved via the `-P 10` pipelining flag, which minimizes syscalls.

### How to Run:
```bash
# Compile
g++ main.cpp Server.cpp -o redis-server

# Run
./redis-server
```
