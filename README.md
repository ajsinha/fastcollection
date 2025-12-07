# FastCollection

**Ultra High-Performance Memory-Mapped Collections with TTL Support**

[![License](https://img.shields.io/badge/License-Proprietary-red.svg)](LICENSE)
[![Patent Pending](https://img.shields.io/badge/Patent-Pending-blue.svg)](#legal)

Copyright © 2025-2030, Ashutosh Sinha (ajsinha@gmail.com)  
**Patent Pending** - All Rights Reserved

---

## Overview

FastCollection is an enterprise-grade library providing ultra high-performance collections that are backed by memory-mapped files. Unlike traditional in-memory collections, FastCollection offers:

| Feature | Standard Collections | FastCollection |
|---------|---------------------|----------------|
| **Persistence** | ❌ Data lost on restart | ✅ Automatic persistence |
| **IPC** | ❌ Single process only | ✅ Multi-process sharing |
| **TTL Support** | ❌ Manual expiration | ✅ Built-in TTL per element |
| **Performance** | ~100ns | **~100-500ns** |
| **Memory Efficiency** | JVM heap | Memory-mapped (OS managed) |

## Key Features

### 🚀 Ultra High Performance
- **100-500ns** average operation time
- **5-10M+ ops/sec** concurrent throughput
- Lock-free reads where possible
- Cache-line aligned structures (64 bytes)

### 💾 Memory-Mapped Architecture
- Data stored in files, mapped to memory
- OS handles page management
- Survives process restarts automatically
- Enables inter-process communication

### ⏰ TTL (Time-To-Live) Support
- Per-element expiration times
- Automatic lazy cleanup
- TTL = -1 means element never expires (default)
- Ideal for caches, rate limiters, sessions

### 📦 Complete Collection Suite
- **FastList** - Doubly-linked list with O(1) head/tail ops
- **FastSet** - Hash set with O(1) lookups
- **FastMap** - Key-value store with atomic operations
- **FastQueue** - FIFO queue with deque operations
- **FastStack** - LIFO stack with lock-free push/pop

## Quick Start

### Java

```java
// Create a list with TTL support
try (FastCollectionList<MyObject> list = 
        new FastCollectionList<>("/tmp/mylist.fc", MyObject.class)) {
    
    // Add element with 5-minute TTL
    list.add(new MyObject("data"), 300);
    
    // Add element without TTL (never expires)
    list.add(new MyObject("permanent"));
    
    // Get element (returns null if expired)
    MyObject obj = list.get(0);
    
    // Check remaining TTL
    long remaining = list.getTTL(0);  // -1 if infinite
    
    // Remove expired elements
    int removed = list.removeExpired();
}
```

### C++

```cpp
#include "fastcollection.h"

// Create a list
FastList list("/tmp/mylist.fc", 64*1024*1024, true);

// Add with 5-minute TTL
std::string data = "hello";
list.add((uint8_t*)data.data(), data.size(), 300);

// Add without TTL (TTL = -1, never expires)
list.add((uint8_t*)data.data(), data.size());

// Get element
std::vector<uint8_t> result;
if (list.get(0, result)) {
    // Use result...
}

// Check TTL
int64_t ttl = list.getTTL(0);  // -1 if infinite
```

### Python

```python
from fastcollection import FastList

# Create a list
lst = FastList("/tmp/mylist.fc")

# Add with 5-minute TTL
lst.add(b"data", ttl=300)

# Add without TTL (never expires)
lst.add(b"permanent")

# Get element
data = lst.get(0)

# Check TTL
remaining = lst.get_ttl(0)  # -1 if infinite
```

## TTL (Time-To-Live) Feature

The TTL feature allows elements to automatically expire after a specified duration. This is particularly useful for:

- **Caching**: Set TTL to match your cache invalidation strategy
- **Rate Limiting**: Track API calls with 60-second TTL
- **Session Management**: Session tokens with 30-minute timeout
- **Temporary Data**: Auto-cleanup of transient data

### TTL Values

| Value | Meaning |
|-------|---------|
| `-1` (default) | Element never expires |
| `0` | Element expires immediately (for testing) |
| `> 0` | Element expires after N seconds |

### TTL Operations

```java
// Add with TTL
list.add(element, 300);           // Expires in 5 minutes

// Check remaining TTL
long remaining = list.getTTL(0);  // Returns: seconds remaining, -1 if infinite, 0 if expired

// Update TTL
list.setTTL(0, 600);              // Extend to 10 minutes

// Remove expired elements
int removed = list.removeExpired();

// Iterate with TTL info
list.forEachWithTTL((data, size, ttl) -> {
    System.out.println("TTL: " + ttl + " seconds");
    return true;  // Continue
});
```

## Collections Reference

### FastList

```java
FastCollectionList<T> list = new FastCollectionList<>(path, Class<T>);

// Add operations (with optional TTL)
list.add(element);                    // Add to end, no TTL
list.add(element, 300);               // Add to end, 5-min TTL
list.add(index, element);             // Add at index, no TTL
list.add(index, element, 300);        // Add at index, 5-min TTL
list.addFirst(element, 300);          // Add to front, 5-min TTL

// Get operations
T element = list.get(index);          // null if expired
T first = list.getFirst();
T last = list.getLast();

// TTL operations
long ttl = list.getTTL(index);        // -1=infinite, 0=expired
list.setTTL(index, 600);              // Update TTL
list.removeExpired();                 // Cleanup
```

### FastMap

```java
FastCollectionMap<K, V> map = new FastCollectionMap<>(path);

// Put operations
map.put(key, value);                  // No TTL
map.put(key, value, 300);             // 5-minute TTL
map.putIfAbsent(key, value, 300);     // Atomic add

// Get operations
V value = map.get(key);               // null if expired
boolean exists = map.containsKey(key);

// TTL operations
long ttl = map.getTTL(key);
map.setTTL(key, 600);
map.removeExpired();
```

### FastSet

```java
FastCollectionSet<T> set = new FastCollectionSet<>(path);

// Add/remove
set.add(element);                     // No TTL
set.add(element, 300);                // 5-minute TTL
set.remove(element);
boolean exists = set.contains(element);

// TTL operations
long ttl = set.getTTL(element);
set.setTTL(element, 600);
set.removeExpired();
```

### FastQueue

```java
FastCollectionQueue<T> queue = new FastCollectionQueue<>(path);

// Queue operations
queue.offer(element);                 // Add to tail, no TTL
queue.offer(element, 300);            // Add to tail, 5-min TTL
T head = queue.poll();                // Remove from head (skips expired)
T peek = queue.peek();

// Deque operations
queue.offerFirst(element, 300);       // Add to front
T tail = queue.pollLast();

// TTL
long ttl = queue.peekTTL();
queue.removeExpired();
```

### FastStack

```java
FastCollectionStack<T> stack = new FastCollectionStack<>(path);

// Stack operations
stack.push(element);                  // No TTL
stack.push(element, 300);             // 5-minute TTL
T top = stack.pop();                  // Remove top (skips expired)
T peek = stack.peek();

// Search
int distance = stack.search(element); // 1-based from top

// TTL
long ttl = stack.peekTTL();
stack.removeExpired();
```

## Performance Characteristics

| Collection | Operation | Average Time | Complexity |
|------------|-----------|--------------|------------|
| **FastList** | add (tail) | < 500ns | O(1) |
| | get (sequential) | < 100ns | O(1) |
| | get (random) | < 1µs | O(n) |
| **FastSet** | add | < 300ns | O(1) avg |
| | contains | < 100ns | O(1) avg |
| **FastMap** | put | < 400ns | O(1) avg |
| | get | < 150ns | O(1) avg |
| **FastQueue** | offer/poll | < 200ns | O(1) |
| **FastStack** | push/pop | < 150ns | O(1) |

## Building

### Prerequisites

- CMake 3.16+
- C++20 compatible compiler
- Boost 1.75+ (interprocess)
- Maven 3.6+ (for Java)
- JDK 11+
- Python 3.8+ (for bindings)

### Build Commands

```bash
# Build everything
mvn clean install

# Build C++ only
cd src/main/cpp
mkdir build && cd build
cmake ..
make -j8

# Run tests
mvn test
```

## Legal

**Copyright © 2025-2030, Ashutosh Sinha (ajsinha@gmail.com)**

This software and its documentation are proprietary and confidential. Unauthorized copying, distribution, modification, or use is strictly prohibited without explicit written permission from the copyright holder.

**Patent Pending**: The architectural patterns and implementations described in this library may be subject to patent applications. Commercial use requires licensing.

## Support

For licensing inquiries or technical support, contact: ajsinha@gmail.com
