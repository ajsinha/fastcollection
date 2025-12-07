# FastCollection v1.0.0 Examples

This directory contains runnable examples for FastCollection in Java, Python, and C++.

## Java Examples

### Prerequisites
- Java 17+
- FastCollection JAR in classpath

### Building & Running

```bash
cd examples/java

# Compile all examples
javac -cp ../../target/fastcollection-1.0.0.jar *.java

# Run individual examples
java -cp .:../../target/fastcollection-1.0.0.jar examples.BasicExample
java -cp .:../../target/fastcollection-1.0.0.jar examples.CacheExample
java -cp .:../../target/fastcollection-1.0.0.jar examples.SessionManagerExample
java -cp .:../../target/fastcollection-1.0.0.jar examples.RateLimiterExample
java -cp .:../../target/fastcollection-1.0.0.jar examples.TaskQueueExample
java -cp .:../../target/fastcollection-1.0.0.jar examples.EventLogExample
java -cp .:../../target/fastcollection-1.0.0.jar examples.LeaderboardExample
java -cp .:../../target/fastcollection-1.0.0.jar examples.ShoppingCartExample
java -cp .:../../target/fastcollection-1.0.0.jar examples.DistributedLockExample
```

### Available Examples

| File | Description |
|------|-------------|
| `BasicExample.java` | Basic list operations (add, get, remove, iterate) |
| `CacheExample.java` | Key-value cache with TTL and expiry |
| `SessionManagerExample.java` | Session management with auto-expiry |
| `RateLimiterExample.java` | API rate limiting implementation |
| `TaskQueueExample.java` | Persistent task queue with dead letter queue |
| `EventLogExample.java` | Event logging with retention and filtering |
| `LeaderboardExample.java` | Game leaderboard with rankings |
| `ShoppingCartExample.java` | E-commerce shopping cart with coupons |
| `DistributedLockExample.java` | Distributed locking mechanism |

## Python Examples

### Prerequisites
- Python 3.8+
- `pip install fastcollection`

### Running

```bash
cd examples/python

# Run individual examples
python basic_example.py
python cache_example.py
python task_queue_example.py
python session_manager_example.py
python rate_limiter_example.py
python event_log_example.py
```

### Available Examples

| File | Description |
|------|-------------|
| `basic_example.py` | Basic list operations with context manager |
| `cache_example.py` | JSON-based cache with TTL |
| `task_queue_example.py` | Task queue with priorities and DLQ |
| `session_manager_example.py` | Session management with auto-expiry |
| `rate_limiter_example.py` | Single and multi-tier rate limiting |
| `event_log_example.py` | Event logging with search and filtering |

## C++ Examples

### Prerequisites
- C++20 compiler (GCC 10+, Clang 13+)
- Boost libraries
- FastCollection native library

### Building & Running

```bash
cd examples/cpp

# Linux
g++ -std=c++20 -O3 -I../../src/main/cpp/include \
    basic_example.cpp -o basic_example \
    -L../../target/native/linux/x64 -lfastcollection_cpp-linux-x64 \
    -lboost_system -lpthread -lrt

g++ -std=c++20 -O3 -I../../src/main/cpp/include \
    cache_example.cpp -o cache_example \
    -L../../target/native/linux/x64 -lfastcollection_cpp-linux-x64 \
    -lboost_system -lpthread -lrt

g++ -std=c++20 -O3 -I../../src/main/cpp/include \
    task_queue_example.cpp -o task_queue_example \
    -L../../target/native/linux/x64 -lfastcollection_cpp-linux-x64 \
    -lboost_system -lpthread -lrt

# Run
LD_LIBRARY_PATH=../../target/native/linux/x64 ./basic_example
LD_LIBRARY_PATH=../../target/native/linux/x64 ./cache_example
LD_LIBRARY_PATH=../../target/native/linux/x64 ./task_queue_example

# macOS
clang++ -std=c++20 -O3 -I../../src/main/cpp/include \
    basic_example.cpp -o basic_example \
    -L../../target/native/macos/arm64 -lfastcollection_cpp-macos-arm64 \
    -lboost_system

DYLD_LIBRARY_PATH=../../target/native/macos/arm64 ./basic_example
```

### Available Examples

| File | Description |
|------|-------------|
| `basic_example.cpp` | Basic list operations with RAII |
| `cache_example.cpp` | Key-value cache with TTL |
| `task_queue_example.cpp` | Task queue with DLQ support |

## More Examples

For more comprehensive examples with detailed explanations, see [docs/QUICKSTART.md](../docs/QUICKSTART.md) which includes:

- **15+ Java examples** - Full implementations with all use cases
- **10+ Python examples** - Idiomatic Python with dataclasses
- **8+ C++ examples** - Modern C++20 with RAII patterns

---

**FastCollection v1.0.0** - Examples
