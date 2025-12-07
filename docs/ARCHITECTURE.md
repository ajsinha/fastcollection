# FastCollection v1.0.0 Architecture

**Technical Deep-Dive into the FastCollection System**

Copyright © 2025-2030, Ashutosh Sinha (ajsinha@gmail.com) - Patent Pending

---

## Table of Contents

1. [System Overview](#system-overview)
2. [Memory-Mapped File Architecture](#memory-mapped-file-architecture)
3. [Data Structures](#data-structures)
4. [TTL Implementation](#ttl-implementation)
5. [Concurrency Model](#concurrency-model)
6. [Cross-Language Bindings](#cross-language-bindings)
7. [Performance Optimizations](#performance-optimizations)

---

## System Overview

FastCollection is built on three core principles:

1. **Memory-Mapped Files**: All data resides in files mapped into virtual memory
2. **Lock-Free Operations**: Where possible, use CAS and atomics for concurrency
3. **TTL-Aware**: Every element can have an optional expiration time

### Architecture Layers

```
┌─────────────────────────────────────────────────────────────────┐
│                     Application Layer                            │
│              (Java / Python / Direct C++)                        │
├─────────────────────────────────────────────────────────────────┤
│                     Language Bindings                            │
│                    (JNI / pybind11)                              │
├─────────────────────────────────────────────────────────────────┤
│                     C++ Core Library                             │
│   ┌───────────┬───────────┬───────────┬───────────┬───────────┐ │
│   │ FastList  │ FastSet   │ FastMap   │ FastQueue │ FastStack │ │
│   └───────────┴───────────┴───────────┴───────────┴───────────┘ │
├─────────────────────────────────────────────────────────────────┤
│                  Serialization & TTL Layer                       │
│         (ShmEntry, ShmNode, ShmKeyValue with TTL)               │
├─────────────────────────────────────────────────────────────────┤
│               Memory-Mapped File Manager                         │
│                 (MMapFileManager)                                │
├─────────────────────────────────────────────────────────────────┤
│              Boost.Interprocess / OS Layer                       │
│         (managed_mapped_file, mutexes, atomics)                 │
├─────────────────────────────────────────────────────────────────┤
│                    Operating System                              │
│               (Memory Manager, File System)                      │
└─────────────────────────────────────────────────────────────────┘
```

---

## Memory-Mapped File Architecture

### Why Memory-Mapped Files?

Unlike traditional shared memory segments (which require explicit naming and can leak), FastCollection uses **memory-mapped files** via `boost::interprocess::managed_mapped_file`. Benefits:

| Feature | Shared Memory | Memory-Mapped Files |
|---------|--------------|---------------------|
| Persistence | ❌ Lost on reboot | ✅ Persists to disk |
| Naming | System-wide names | File paths |
| Size limits | OS-dependent | Disk space |
| Cleanup | Manual | Automatic (files) |
| IPC | Yes | Yes |
| Debugging | Hard | Easy (file inspection) |

### File Structure

Each collection creates a single memory-mapped file:

```
┌─────────────────────────────────────────────────────────────┐
│                    File Header                               │
│  ┌─────────────────────────────────────────────────────────┐ │
│  │ Magic Number (0xFAC0LLEC) │ Version │ Timestamps        │ │
│  │ Size (atomic) │ Capacity │ Global Mutex                 │ │
│  └─────────────────────────────────────────────────────────┘ │
├─────────────────────────────────────────────────────────────┤
│                Collection-Specific Header                    │
│  (ListHeader / HashTableHeader / DequeHeader)               │
├─────────────────────────────────────────────────────────────┤
│                       Data Region                            │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐                     │
│  │ ShmNode  │ │ ShmNode  │ │ShmKeyValue│  ...               │
│  │ (with    │ │ (with    │ │ (with    │                     │
│  │  TTL)    │ │  TTL)    │ │  TTL)    │                     │
│  └──────────┘ └──────────┘ └──────────┘                     │
├─────────────────────────────────────────────────────────────┤
│                     Free Space Pool                          │
│            (Managed by Boost allocator)                      │
└─────────────────────────────────────────────────────────────┘
```

### Constructor Pattern

All collections follow the same constructor pattern:

```cpp
// File path is ALWAYS the first parameter
FastList list("/path/to/mylist.fc", initial_size, create_new);
//            ↑ File path          ↑ Bytes       ↑ Truncate?
```

- **file_path**: Full path to the memory-mapped file
- **initial_size**: Initial file size (grows automatically)
- **create_new**: If true, truncates existing file

---

## Data Structures

### ShmEntry - The Foundation

Every element is wrapped in `ShmEntry`, which provides:
- State management (empty, writing, valid, deleted, expired)
- TTL tracking (creation time, expiration time)
- Hash code caching
- Version for optimistic locking

```cpp
struct alignas(64) ShmEntry {  // 64-byte aligned for cache
    std::atomic<uint32_t> state;    // STATE_EMPTY/WRITING/VALID/DELETED/EXPIRED
    uint32_t data_size;             // Size of serialized data
    uint32_t hash_code;             // For quick equality checks
    int32_t ttl_seconds;            // TTL: -1=infinite, 0=immediate, >0=seconds
    uint64_t created_at;            // Nanosecond timestamp
    uint64_t expires_at;            // 0 = never expires
    uint64_t version;               // For optimistic locking
    
    // Key methods
    bool is_alive() const;          // Valid and not expired
    bool is_expired() const;        // Check expiration
    void set_ttl(int32_t ttl);      // Set TTL and calculate expires_at
    int64_t remaining_ttl_seconds(); // Get remaining TTL
};
```

### ShmNode - For Lists/Queues/Stacks

```cpp
struct ShmNode {
    ShmEntry entry;                      // Contains TTL info
    std::atomic<int64_t> next_offset;    // Offset to next node
    std::atomic<int64_t> prev_offset;    // Offset to prev node
    uint8_t data[0];                     // Flexible array for data
};
```

### ShmKeyValue - For Maps

```cpp
struct ShmKeyValue {
    ShmEntry entry;          // Contains TTL info
    uint32_t key_size;
    uint32_t value_size;
    uint8_t data[0];         // key bytes, then value bytes
};
```

### Offset-Based Pointers

Since memory-mapped files can be loaded at different addresses in different processes, we use **offsets** instead of pointers:

```cpp
// Convert offset to pointer
ShmNode* node_at_offset(int64_t offset) {
    if (offset < 0) return nullptr;
    void* base = file_manager_->segment_manager();
    return reinterpret_cast<ShmNode*>(
        static_cast<uint8_t*>(base) + offset
    );
}

// Convert pointer to offset
int64_t offset = static_cast<uint8_t*>(ptr) - static_cast<uint8_t*>(base);
```

---

## TTL Implementation

### Design Philosophy

The TTL feature follows these principles:

1. **Default is Infinite**: TTL = -1 means element never expires
2. **Lazy Cleanup**: Expired elements are cleaned during operations
3. **Lock-Free Checks**: Expiration checks use atomic reads
4. **Per-Element TTL**: Each element can have a different TTL

### TTL Storage

```cpp
void ShmEntry::set_ttl(int32_t ttl) {
    ttl_seconds = ttl;
    created_at = current_timestamp_ns();  // Nanoseconds since epoch
    
    if (ttl < 0) {
        expires_at = 0;  // 0 = never expires
    } else {
        // Convert seconds to nanoseconds and add to creation time
        expires_at = created_at + (static_cast<uint64_t>(ttl) * 1000000000ULL);
    }
}
```

### Expiration Checking

```cpp
bool ShmEntry::is_expired() const {
    uint32_t s = state.load(std::memory_order_acquire);
    if (s == STATE_EXPIRED) return true;
    if (s != STATE_VALID) return false;
    if (expires_at == 0) return false;  // Never expires
    return current_timestamp_ns() >= expires_at;
}

bool ShmEntry::is_alive() const {
    if (!is_valid()) return false;
    if (expires_at == 0) return true;   // No expiration set
    return current_timestamp_ns() < expires_at;
}
```

### Remaining TTL Calculation

```cpp
int64_t ShmEntry::remaining_ttl_seconds() const {
    if (ttl_seconds < 0) return -1;      // Infinite
    if (expires_at == 0) return -1;
    
    uint64_t now = current_timestamp_ns();
    if (now >= expires_at) return 0;     // Already expired
    
    return static_cast<int64_t>((expires_at - now) / 1000000000ULL);
}
```

### Cleanup Strategies

FastCollection uses **lazy cleanup** - expired elements are removed during:

1. **Read operations**: Skip expired elements
2. **Write operations**: Replace expired elements
3. **Explicit cleanup**: `removeExpired()` method

```cpp
// Example: Lazy cleanup during poll
bool FastQueue::poll(std::vector<uint8_t>& out_data) {
    while (true) {
        ShmNode* front = get_front_node();
        if (!front) return false;  // Empty
        
        if (front->entry.is_expired()) {
            // Remove expired node and continue
            remove_front_node();
            continue;
        }
        
        // Found valid non-expired node
        out_data = copy_data(front);
        remove_front_node();
        return true;
    }
}
```

---

## Concurrency Model

### Lock Hierarchy

```
Global Mutex (structural changes)
        │
        ├── List: All operations
        ├── Queue: All operations  
        ├── Stack: removeExpired, removeElement
        │
Bucket Mutex (per-bucket, Set/Map)
        │
        ├── Set: Per-bucket add/remove
        └── Map: Per-bucket put/remove

Lock-Free Operations
        │
        ├── Stack: push, pop (CAS)
        └── All: contains, getTTL (read-only)
```

### Lock-Free Stack Push

```cpp
bool FastStack::push(const uint8_t* data, size_t size, int32_t ttl_seconds) {
    ShmNode* node = allocate_node(size);
    copy_data_to_node(node, data, size, ttl_seconds);
    
    while (true) {
        int64_t old_top = header_->front_offset.load(std::memory_order_acquire);
        node->next_offset.store(old_top, std::memory_order_release);
        
        if (header_->front_offset.compare_exchange_weak(
                old_top, node_offset,
                std::memory_order_acq_rel,
                std::memory_order_relaxed)) {
            
            // Success - increment ABA tag
            aba_tag_->fetch_add(1, std::memory_order_relaxed);
            return true;
        }
        // CAS failed, retry
    }
}
```

### Per-Bucket Locking (Set/Map)

```cpp
bool FastSet::add(const uint8_t* data, size_t size, int32_t ttl_seconds) {
    uint32_t hash = compute_hash(data, size);
    ShmBucket* bucket = get_bucket(hash);
    
    // Lock only this bucket
    IpcScopedLock lock(bucket->mutex);
    
    // Check for existing (lock-free within bucket)
    ShmNode* existing = find_in_bucket(bucket, data, size, hash);
    if (existing && existing->entry.is_alive()) {
        return false;  // Already exists
    }
    
    // Add new node
    // ...
}
```

---

## Cross-Language Bindings

### JNI Layer

```cpp
// Example JNI method
JNIEXPORT jboolean JNICALL Java_...FastCollectionList_nativeAdd
  (JNIEnv* env, jobject obj, jlong handle, jbyteArray data, jint ttlSeconds) {
    try {
        FastList* list = reinterpret_cast<FastList*>(handle);
        
        // RAII byte array access
        JByteArrayAccess dataAccess(env, data);
        if (!dataAccess.valid()) return JNI_FALSE;
        
        return list->add(dataAccess.data(), dataAccess.length(), ttlSeconds) 
               ? JNI_TRUE : JNI_FALSE;
    } catch (const std::exception& e) {
        throwException(env, e.what());
        return JNI_FALSE;
    }
}
```

### Java Wrapper

```java
public class FastCollectionList<T extends Serializable> {
    private final long nativeHandle;
    
    // Native method declaration
    private native boolean nativeAdd(long handle, byte[] data, int ttlSeconds);
    
    // Java API
    public boolean add(T element, int ttlSeconds) {
        checkClosed();
        byte[] data = serialize(element);
        return nativeAdd(nativeHandle, data, ttlSeconds);
    }
}
```

---

## Performance Optimizations

### 1. Cache-Line Alignment

All structures are aligned to 64 bytes to prevent false sharing:

```cpp
struct alignas(64) ShmEntry { ... };
```

### 2. Prefetching

```cpp
ShmNode* node_at_offset(int64_t offset) {
    ShmNode* node = ...;
    
    // Prefetch next node for linked traversal
    if (node->next_offset >= 0) {
        prefetch_read(base + node->next_offset);
    }
    return node;
}
```

### 3. Hash Caching

```cpp
// Hash computed once and stored
node->entry.hash_code = compute_hash(data, size);

// Fast equality check
if (node->entry.hash_code != target_hash) continue;  // Skip expensive memcmp
if (memcmp(node->data, target, size) == 0) { ... }   // Only if hash matches
```

### 4. Sequential Access Optimization (List)

```cpp
struct AccessCache {
    size_t last_index = SIZE_MAX;
    int64_t last_offset = -1;
};

// Detect sequential access pattern
if (index == access_cache.last_index + 1) {
    // Start from cached position instead of head
    return follow_next_pointer(access_cache.last_offset);
}
```

---

## File Locations

| File | Description |
|------|-------------|
| `src/main/cpp/include/fc_common.h` | Common definitions, TTL constants |
| `src/main/cpp/include/fc_serialization.h` | ShmEntry, ShmNode with TTL |
| `src/main/cpp/include/fc_*.h` | Collection headers |
| `src/main/cpp/src/fc_*.cpp` | Collection implementations |
| `src/main/cpp/jni/jni_*.cpp` | JNI bindings |
| `src/main/java/.../FastCollection*.java` | Java wrappers |

---

*Copyright © 2025-2030, Ashutosh Sinha - Patent Pending*
