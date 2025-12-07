# FastCollection TTL (Time-To-Live) Guide

Copyright © 2025-2030, Ashutosh Sinha (ajsinha@gmail.com)  
Patent Pending

## Overview

FastCollection provides per-element TTL (Time-To-Live) support across all collection types. Elements can be configured to automatically expire after a specified duration, enabling efficient cache-like behavior with persistent storage.

## TTL Values

| Value | Meaning |
|-------|---------|
| `-1` | **Infinite** - Element never expires (default) |
| `0` | **Immediate expiration** - For testing |
| `> 0` | **Seconds** - Element expires after N seconds |

## Basic Usage

### Java

```java
// List with TTL
FastCollectionList<String> list = new FastCollectionList<>("/tmp/list.fc", String.class);
list.add("permanent");           // Never expires (TTL=-1)
list.add("cached", 300);         // Expires in 5 minutes
list.add("session", 3600);       // Expires in 1 hour

// Map with TTL
FastCollectionMap<String, String> map = new FastCollectionMap<>("/tmp/map.fc", String.class, String.class);
map.put("key", "value");         // Never expires
map.put("temp", "data", 60);     // Expires in 1 minute

// Set with TTL
FastCollectionSet<String> set = new FastCollectionSet<>("/tmp/set.fc", String.class);
set.add("member", 120);          // Expires in 2 minutes

// Queue with TTL
FastCollectionQueue<String> queue = new FastCollectionQueue<>("/tmp/queue.fc", String.class);
queue.offer("task", 600);        // Task expires in 10 minutes

// Stack with TTL
FastCollectionStack<String> stack = new FastCollectionStack<>("/tmp/stack.fc", String.class);
stack.push("item", 30);          // Expires in 30 seconds
```

### Python

```python
from fastcollection import FastList, FastMap, TTL_INFINITE

# List
lst = FastList("/tmp/list.fc")
lst.add(b"permanent")            # Never expires
lst.add(b"cached", ttl=300)      # 5-minute TTL

# Map
m = FastMap("/tmp/map.fc")
m.put(b"key", b"value")          # Never expires  
m.put(b"temp", b"data", ttl=60)  # 1-minute TTL
```

### C++

```cpp
#include "fastcollection.h"
using namespace fastcollection;

FastList list("/tmp/list.fc");
list.add(data, size);                  // Never expires (TTL_INFINITE)
list.add(data, size, 300);             // 5-minute TTL

FastMap map("/tmp/map.fc");
map.put(key, klen, val, vlen);         // Never expires
map.put(key, klen, val, vlen, 60);     // 1-minute TTL
```

## TTL Operations

### Check Remaining TTL

```java
// Returns: -1 (infinite), 0 (expired), or remaining seconds
long remaining = list.getTTL(index);
long remaining = map.getTTL("key");
long remaining = set.getTTL("member");
```

### Update TTL

```java
// Extend TTL
list.setTTL(0, 600);           // Extend to 10 minutes
map.setTTL("key", 3600);       // Extend to 1 hour

// Make permanent
list.setTTL(0, -1);            // Never expires

// Expire immediately (for testing)
list.setTTL(0, 0);
```

### Remove Expired Elements

```java
// Manual cleanup
int removed = list.removeExpired();
int removed = map.removeExpired();
int removed = queue.removeExpired();
```

## Expiration Behavior

### Lazy Cleanup

Expired elements are automatically skipped during read operations:

```java
list.add("temp", 1);  // 1-second TTL
Thread.sleep(2000);   // Wait for expiration

// These operations skip expired elements:
list.size();          // Returns 0
list.get(0);          // Returns null
queue.poll();         // Returns null (skips expired front)
```

### Size Calculation

The `size()` method returns the count of **non-expired** elements:

```java
list.add("a", 1);      // Expires in 1 sec
list.add("b", -1);     // Never expires
list.add("c", 1);      // Expires in 1 sec

list.size();           // Returns 3

Thread.sleep(2000);

list.size();           // Returns 1 (only "b" remains)
```

### Iteration

Iterators automatically skip expired elements:

```java
list.add("expired", 1);
list.add("valid", -1);

Thread.sleep(2000);

for (String s : list) {
    System.out.println(s);  // Only prints "valid"
}
```

## Internal Implementation

### Storage Structure

Each element stores TTL metadata in `ShmEntry`:

```cpp
struct ShmEntry {
    // ... other fields ...
    int32_t  ttl_seconds;    // -1 = infinite
    uint64_t created_at;     // Nanosecond timestamp
    uint64_t expires_at;     // 0 = never, else absolute ns
};
```

### Expiration Check

```cpp
bool is_alive() const {
    if (expires_at == 0) return true;  // Never expires
    return current_timestamp_ns() < expires_at;
}
```

### Performance

TTL checking is O(1) and lock-free:
- No background threads or timers
- Atomic timestamp comparison
- Nanosecond precision

## Best Practices

### 1. Use Appropriate TTLs

```java
// Session data: 30 minutes
map.put("session:" + sessionId, userData, 1800);

// Cache: 5 minutes
map.put("cache:" + key, computed, 300);

// Permanent config: infinite
map.put("config", settings, -1);
```

### 2. Periodic Cleanup

For memory efficiency, periodically remove expired entries:

```java
// In a scheduled task
ScheduledExecutorService scheduler = Executors.newScheduledThreadPool(1);
scheduler.scheduleAtFixedRate(() -> {
    map.removeExpired();
}, 1, 1, TimeUnit.MINUTES);
```

### 3. Check TTL Before Critical Operations

```java
long ttl = map.getTTL("important-key");
if (ttl > 0 && ttl < 60) {
    // Expiring soon - extend or refresh
    map.setTTL("important-key", 3600);
}
```

### 4. Handle Null Returns

```java
String value = map.get("key");
if (value == null) {
    // Either key doesn't exist OR has expired
    value = computeAndStore("key");
}
```

## Queue/Stack TTL Behavior

### Queue (FIFO)

When polling from a queue, expired elements at the front are automatically skipped:

```java
queue.offer("expired", 1);
queue.offer("valid", -1);

Thread.sleep(2000);

queue.poll();  // Returns "valid" (skipped expired front)
```

### Stack (LIFO)

When popping from a stack, expired top elements are skipped:

```java
stack.push("valid", -1);
stack.push("expired", 1);

Thread.sleep(2000);

stack.pop();  // Returns "valid" (skipped expired top)
```

## Persistence

TTL survives process restarts:

```java
// Process 1: Create with TTL
FastCollectionMap<String, String> map = new FastCollectionMap<>("/tmp/map.fc", ...);
map.put("key", "value", 3600);  // 1 hour TTL
map.close();

// Process 2: Reopen - TTL continues from original creation time
FastCollectionMap<String, String> map = new FastCollectionMap<>("/tmp/map.fc", ...);
long remaining = map.getTTL("key");  // Shows remaining time
```

## Thread Safety

All TTL operations are thread-safe:
- `getTTL()` - Lock-free atomic read
- `setTTL()` - Uses same locking as write operations
- `removeExpired()` - Uses collection's locking strategy
- Expiration checks - Lock-free atomic comparisons
