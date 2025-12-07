# FastCollection v1.0.0 API Reference

Copyright © 2025-2030, Ashutosh Sinha (ajsinha@gmail.com)  
Patent Pending

---

## Java API

### FastCollectionList<T>

```java
// Constructor
FastCollectionList(String filePath, Class<T> type)
FastCollectionList(String filePath, Class<T> type, long initialSize, boolean createNew)

// Add elements
boolean add(T element)                    // TTL = infinite
boolean add(T element, int ttlSeconds)    // Custom TTL
boolean add(int index, T element)
boolean add(int index, T element, int ttlSeconds)
void addFirst(T element)
void addFirst(T element, int ttlSeconds)

// Get elements
T get(int index)
T getFirst()
T getLast()

// Update elements
T set(int index, T element)
T set(int index, T element, int ttlSeconds)

// Remove elements
T remove(int index)
T removeFirst()
T removeLast()
boolean remove(Object o)
void clear()
int removeExpired()

// Query
boolean contains(Object o)
int indexOf(Object o)
int lastIndexOf(Object o)
int size()
boolean isEmpty()

// TTL operations
long getTTL(int index)          // -1=infinite, 0=expired, >0=remaining
boolean setTTL(int index, int ttlSeconds)

// Persistence
void flush()
void close()
```

### FastCollectionMap<K, V>

```java
// Constructor
FastCollectionMap(String filePath, Class<K> keyType, Class<V> valueType)
FastCollectionMap(String filePath, Class<K> keyType, Class<V> valueType, 
                  long initialSize, boolean createNew)

// Put operations
V put(K key, V value)
V put(K key, V value, int ttlSeconds)
boolean putIfAbsent(K key, V value)
boolean putIfAbsent(K key, V value, int ttlSeconds)
boolean replace(K key, V value)
boolean replace(K key, V value, int ttlSeconds)

// Get operations
V get(Object key)
V getOrDefault(Object key, V defaultValue)

// Remove operations
V remove(Object key)
void clear()
int removeExpired()

// Query
boolean containsKey(Object key)
boolean containsValue(Object value)
int size()
boolean isEmpty()

// TTL operations
long getTTL(K key)
boolean setTTL(K key, int ttlSeconds)

// Persistence
void flush()
void close()
```

### FastCollectionSet<T>

```java
// Constructor
FastCollectionSet(String filePath, Class<T> type)
FastCollectionSet(String filePath, Class<T> type, long initialSize, boolean createNew)

// Add/Remove
boolean add(T element)
boolean add(T element, int ttlSeconds)
boolean remove(Object o)
void clear()
int removeExpired()

// Query
boolean contains(Object o)
int size()
boolean isEmpty()

// TTL operations
long getTTL(T element)
boolean setTTL(T element, int ttlSeconds)

// Persistence
void flush()
void close()
```

### FastCollectionQueue<T>

```java
// Constructor
FastCollectionQueue(String filePath, Class<T> type)
FastCollectionQueue(String filePath, Class<T> type, long initialSize, boolean createNew)

// Queue operations
boolean offer(T element)
boolean offer(T element, int ttlSeconds)
boolean offerFirst(T element)
boolean offerFirst(T element, int ttlSeconds)
T poll()
T pollLast()
T peek()
T peekLast()

// Deque aliases
void addFirst(T e)
void addLast(T e)
T removeFirst()
T removeLast()
T getFirst()
T getLast()

// Management
void clear()
int size()
boolean isEmpty()
long peekTTL()
int removeExpired()

// Persistence
void flush()
void close()
```

### FastCollectionStack<T>

```java
// Constructor
FastCollectionStack(String filePath, Class<T> type)
FastCollectionStack(String filePath, Class<T> type, long initialSize, boolean createNew)

// Stack operations
T push(T item)
T push(T item, int ttlSeconds)
T pop()
T peek()
int search(Object o)    // 1-based from top, -1 if not found

// Management
void clear()
int size()
boolean isEmpty()
boolean empty()
long peekTTL()
int removeExpired()

// Persistence
void flush()
void close()
```

## Python API

### FastList

```python
lst = FastList(file_path, initial_size=67108864, create_new=False)

lst.add(data: bytes, ttl: int = -1) -> bool
lst.add_at(index: int, data: bytes, ttl: int = -1) -> bool
lst.add_first(data: bytes, ttl: int = -1) -> bool
lst.get(index: int) -> bytes | None
lst.get_first() -> bytes | None
lst.get_last() -> bytes | None
lst.set(index: int, data: bytes, ttl: int = -1) -> bool
lst.remove(index: int) -> bytes | None
lst.remove_first() -> bytes | None
lst.remove_last() -> bytes | None
lst.contains(data: bytes) -> bool
lst.index_of(data: bytes) -> int
lst.get_ttl(index: int) -> int
lst.set_ttl(index: int, ttl_seconds: int) -> bool
lst.remove_expired() -> int
lst.clear()
lst.size() -> int
lst.is_empty() -> bool
lst.flush()
lst.close()
```

### FastMap

```python
m = FastMap(file_path, initial_size=67108864, create_new=False, bucket_count=16384)

m.put(key: bytes, value: bytes, ttl: int = -1) -> bool
m.put_if_absent(key: bytes, value: bytes, ttl: int = -1) -> bool
m.get(key: bytes) -> bytes | None
m.remove(key: bytes) -> bool
m.contains_key(key: bytes) -> bool
m.get_ttl(key: bytes) -> int
m.set_ttl(key: bytes, ttl_seconds: int) -> bool
m.remove_expired() -> int
m.clear()
m.size() -> int
m.is_empty() -> bool
m.flush()
m.close()

# Dict-like access
m[key] = value
value = m[key]
key in m
```

### FastSet

```python
s = FastSet(file_path, initial_size=67108864, create_new=False, bucket_count=16384)

s.add(data: bytes, ttl: int = -1) -> bool
s.remove(data: bytes) -> bool
s.contains(data: bytes) -> bool
s.get_ttl(data: bytes) -> int
s.set_ttl(data: bytes, ttl_seconds: int) -> bool
s.remove_expired() -> int
s.clear()
s.size() -> int
s.is_empty() -> bool
s.flush()
s.close()

# Set-like access
data in s
```

### FastQueue

```python
q = FastQueue(file_path, initial_size=67108864, create_new=False)

q.offer(data: bytes, ttl: int = -1) -> bool
q.offer_first(data: bytes, ttl: int = -1) -> bool
q.poll() -> bytes | None
q.poll_last() -> bytes | None
q.peek() -> bytes | None
q.peek_ttl() -> int
q.remove_expired() -> int
q.clear()
q.size() -> int
q.is_empty() -> bool
q.flush()
q.close()
```

### FastStack

```python
st = FastStack(file_path, initial_size=67108864, create_new=False)

st.push(data: bytes, ttl: int = -1) -> bool
st.pop() -> bytes | None
st.peek() -> bytes | None
st.search(data: bytes) -> int
st.peek_ttl() -> int
st.remove_expired() -> int
st.clear()
st.size() -> int
st.is_empty() -> bool
st.flush()
st.close()
```

## C++ API

### FastList

```cpp
FastList(const std::string& file_path, 
         size_t initial_size = DEFAULT_INITIAL_SIZE,
         bool create_new = false);

bool add(const uint8_t* data, size_t size, int32_t ttl = TTL_INFINITE);
bool add(size_t index, const uint8_t* data, size_t size, int32_t ttl = TTL_INFINITE);
bool addFirst(const uint8_t* data, size_t size, int32_t ttl = TTL_INFINITE);
bool get(size_t index, std::vector<uint8_t>& result);
bool getFirst(std::vector<uint8_t>& result);
bool getLast(std::vector<uint8_t>& result);
bool set(size_t index, const uint8_t* data, size_t size, int32_t ttl = TTL_INFINITE);
bool remove(size_t index, std::vector<uint8_t>* result = nullptr);
bool removeFirst(std::vector<uint8_t>* result = nullptr);
bool removeLast(std::vector<uint8_t>* result = nullptr);
bool contains(const uint8_t* data, size_t size);
int64_t indexOf(const uint8_t* data, size_t size);
int64_t getTTL(size_t index);
bool setTTL(size_t index, int32_t ttl_seconds);
size_t removeExpired();
void clear();
size_t size();
bool isEmpty();
void flush();
const std::string& filename();
```

### FastMap

```cpp
FastMap(const std::string& file_path,
        size_t initial_size = DEFAULT_INITIAL_SIZE,
        bool create_new = false,
        uint32_t bucket_count = HashTableHeader::DEFAULT_BUCKET_COUNT);

bool put(const uint8_t* key, size_t key_size,
         const uint8_t* value, size_t value_size,
         int32_t ttl = TTL_INFINITE);
bool putIfAbsent(const uint8_t* key, size_t key_size,
                 const uint8_t* value, size_t value_size,
                 int32_t ttl = TTL_INFINITE);
bool replace(const uint8_t* key, size_t key_size,
             const uint8_t* value, size_t value_size,
             int32_t ttl = TTL_INFINITE);
bool get(const uint8_t* key, size_t key_size, std::vector<uint8_t>& result);
bool remove(const uint8_t* key, size_t key_size);
bool containsKey(const uint8_t* key, size_t key_size);
int64_t getTTL(const uint8_t* key, size_t key_size);
bool setTTL(const uint8_t* key, size_t key_size, int32_t ttl_seconds);
size_t removeExpired();
void clear();
size_t size();
bool isEmpty();
void flush();
```

## TTL Constants

| Constant | Value | Meaning |
|----------|-------|---------|
| `TTL_INFINITE` | -1 | Element never expires (default) |
| `TTL_DEFAULT` | -1 | Alias for TTL_INFINITE |

## Return Values

### getTTL() Returns

| Value | Meaning |
|-------|---------|
| -1 | Element never expires |
| 0 | Element has expired |
| > 0 | Remaining seconds until expiration |
