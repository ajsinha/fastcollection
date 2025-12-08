# FastCollection v1.0.0 Quick Start Guide

**Get started with FastCollection in minutes!**

Copyright © 2025-2030, Ashutosh Sinha (ajsinha@gmail.com) - Patent Pending

---

## Table of Contents

1. [Installation](#installation)
2. [Java Examples](#java-examples)
3. [Python Examples](#python-examples)
4. [C++ Examples](#c-examples)
5. [Common Patterns](#common-patterns)
6. [Best Practices](#best-practices)

---

## Installation

### Maven (Java)

```xml
<dependency>
    <groupId>com.kuber</groupId>
    <artifactId>fastcollection</artifactId>
    <version>1.0.0</version>
</dependency>
```

### Gradle (Java)

```groovy
implementation 'com.kuber:fastcollection:1.0.0'
```

### Python

```bash
# Install from source (requires C++ compiler and Boost)
cd fastcollection
pip install pybind11  # Required build dependency
pip install -e .      # Development install

# Or build wheel
pip install .
```

**Requirements:**
- Python 3.8+
- pybind11 >= 2.6
- C++ compiler with C++20 support
- Boost development headers

### C++ (CMake)

```cmake
find_package(FastCollection REQUIRED)
target_link_libraries(myapp FastCollection::fastcollection)
```

---

## Java Examples

### Example 1: Basic List Operations

```java
import com.kuber.fastcollection.*;

public class BasicListExample {
    public static void main(String[] args) {
        // Create a persistent list (data survives restarts)
        try (FastCollectionList<String> list = 
                new FastCollectionList<>("/tmp/mylist.fc", String.class)) {
            
            // Add elements
            list.add("Hello");
            list.add("World");
            list.add("FastCollection");
            
            // Access elements
            System.out.println("First: " + list.get(0));      // Hello
            System.out.println("Size: " + list.size());        // 3
            
            // Iterate
            for (String item : list) {
                System.out.println("Item: " + item);
            }
            
            // Remove
            list.remove(1);  // Remove "World"
            
            // Check contents
            System.out.println("Contains 'Hello': " + list.contains("Hello"));  // true
            System.out.println("Index of 'FastCollection': " + list.indexOf("FastCollection"));  // 1
            
        }  // Auto-closes and persists data
    }
}
```

### Example 2: List with TTL (Time-To-Live)

```java
import com.kuber.fastcollection.*;

public class ListTTLExample {
    public static void main(String[] args) throws InterruptedException {
        try (FastCollectionList<String> cache = 
                new FastCollectionList<>("/tmp/cache.fc", String.class)) {
            
            // Add items with different TTLs
            cache.add("permanent-data");           // Never expires (TTL = -1)
            cache.add("5-minute-data", 300);       // Expires in 5 minutes
            cache.add("1-hour-data", 3600);        // Expires in 1 hour
            cache.add("10-second-data", 10);       // Expires in 10 seconds
            
            System.out.println("Initial size: " + cache.size());  // 4
            
            // Check TTL values
            System.out.println("TTL[0]: " + cache.getTTL(0));  // -1 (infinite)
            System.out.println("TTL[1]: " + cache.getTTL(1));  // ~300
            System.out.println("TTL[2]: " + cache.getTTL(2));  // ~3600
            System.out.println("TTL[3]: " + cache.getTTL(3));  // ~10
            
            // Wait for short-lived item to expire
            Thread.sleep(12000);  // 12 seconds
            
            // Size automatically excludes expired items
            System.out.println("Size after expiry: " + cache.size());  // 3
            
            // Extend TTL of an item
            cache.setTTL(1, 7200);  // Extend to 2 hours
            
            // Convert to infinite TTL
            cache.setTTL(2, -1);  // Never expires now
            
            // Manually remove expired items (optional - done lazily anyway)
            int removed = cache.removeExpired();
            System.out.println("Removed " + removed + " expired items");
        }
    }
}
```

### Example 3: Key-Value Map with Caching

```java
import com.kuber.fastcollection.*;

public class MapCacheExample {
    public static void main(String[] args) {
        try (FastCollectionMap<String, String> cache = 
                new FastCollectionMap<>("/tmp/cache.fc")) {
            
            // Simple put/get
            cache.put("user:1001", "John Doe");
            cache.put("user:1002", "Jane Smith");
            
            String user = cache.get("user:1001");
            System.out.println("User 1001: " + user);  // John Doe
            
            // Put with TTL (session cache - 30 minutes)
            cache.put("session:abc123", "user_data_here", 1800);
            
            // Put if absent (atomic operation)
            String existing = cache.putIfAbsent("user:1001", "New Name");
            System.out.println("Existing: " + existing);  // John Doe (not replaced)
            
            // Check existence
            if (cache.containsKey("user:1002")) {
                System.out.println("User 1002 exists!");
            }
            
            // Get TTL
            long ttl = cache.getTTL("session:abc123");
            System.out.println("Session TTL: " + ttl + " seconds");
            
            // Update value and reset TTL
            cache.put("session:abc123", "updated_data", 1800);
            
            // Remove
            String removed = cache.remove("user:1002");
            System.out.println("Removed: " + removed);  // Jane Smith
            
            // Size
            System.out.println("Cache size: " + cache.size());
        }
    }
}
```

### Example 4: High-Performance Set Operations

```java
import com.kuber.fastcollection.*;
import java.util.*;

public class SetExample {
    public static void main(String[] args) {
        try (FastCollectionSet<String> uniqueVisitors = 
                new FastCollectionSet<>("/tmp/visitors.fc")) {
            
            // Track unique visitors (with 24-hour TTL)
            int ttl24h = 24 * 60 * 60;  // 86400 seconds
            
            uniqueVisitors.add("visitor_001", ttl24h);
            uniqueVisitors.add("visitor_002", ttl24h);
            uniqueVisitors.add("visitor_003", ttl24h);
            uniqueVisitors.add("visitor_001", ttl24h);  // Duplicate - returns false
            
            System.out.println("Unique visitors: " + uniqueVisitors.size());  // 3
            
            // Check if visitor seen before
            if (uniqueVisitors.contains("visitor_001")) {
                System.out.println("Welcome back, visitor_001!");
            }
            
            // Add batch of visitors
            List<String> newVisitors = Arrays.asList(
                "visitor_004", "visitor_005", "visitor_006"
            );
            for (String v : newVisitors) {
                if (uniqueVisitors.add(v, ttl24h)) {
                    System.out.println("New visitor: " + v);
                }
            }
            
            // Remove visitor
            uniqueVisitors.remove("visitor_002");
            
            // Clear all
            // uniqueVisitors.clear();
        }
    }
}
```

### Example 5: Task Queue with Priority

```java
import com.kuber.fastcollection.*;
import java.io.Serializable;

public class TaskQueueExample {
    
    // Task class must be Serializable
    public static class Task implements Serializable {
        private static final long serialVersionUID = 1L;
        public String id;
        public String type;
        public String payload;
        public int priority;
        
        public Task(String id, String type, String payload, int priority) {
            this.id = id;
            this.type = type;
            this.payload = payload;
            this.priority = priority;
        }
        
        @Override
        public String toString() {
            return "Task{id='" + id + "', type='" + type + "', priority=" + priority + "}";
        }
    }
    
    public static void main(String[] args) {
        try (FastCollectionQueue<Task> taskQueue = 
                new FastCollectionQueue<>("/tmp/tasks.fc")) {
            
            // Producer: Add tasks with TTL (tasks expire if not processed in 1 hour)
            int taskTTL = 3600;
            
            taskQueue.offer(new Task("t1", "email", "Send welcome email", 1), taskTTL);
            taskQueue.offer(new Task("t2", "report", "Generate daily report", 2), taskTTL);
            taskQueue.offer(new Task("t3", "backup", "Backup database", 3), taskTTL);
            
            // Add urgent task to front of queue
            taskQueue.offerFirst(new Task("t0", "alert", "Critical alert!", 0), taskTTL);
            
            System.out.println("Queue size: " + taskQueue.size());  // 4
            
            // Consumer: Process tasks
            Task task;
            while ((task = taskQueue.poll()) != null) {
                System.out.println("Processing: " + task);
                // Process task...
            }
            
            System.out.println("Queue empty: " + taskQueue.isEmpty());  // true
        }
    }
}
```

### Example 6: Undo/Redo Stack

```java
import com.kuber.fastcollection.*;
import java.io.Serializable;

public class UndoStackExample {
    
    public static class Action implements Serializable {
        private static final long serialVersionUID = 1L;
        public String type;
        public String description;
        public long timestamp;
        
        public Action(String type, String description) {
            this.type = type;
            this.description = description;
            this.timestamp = System.currentTimeMillis();
        }
        
        @Override
        public String toString() {
            return type + ": " + description;
        }
    }
    
    public static void main(String[] args) {
        try (FastCollectionStack<Action> undoStack = 
                new FastCollectionStack<>("/tmp/undo.fc")) {
            
            // Perform actions and push to undo stack
            undoStack.push(new Action("INSERT", "Added text 'Hello'"));
            undoStack.push(new Action("FORMAT", "Made text bold"));
            undoStack.push(new Action("INSERT", "Added text 'World'"));
            undoStack.push(new Action("DELETE", "Removed character"));
            
            System.out.println("Actions in stack: " + undoStack.size());  // 4
            
            // Peek at last action
            Action lastAction = undoStack.peek();
            System.out.println("Last action: " + lastAction);  // DELETE: Removed character
            
            // Undo last 2 actions
            System.out.println("\nUndoing:");
            for (int i = 0; i < 2; i++) {
                Action action = undoStack.pop();
                if (action != null) {
                    System.out.println("  Undone: " + action);
                }
            }
            
            // Search for specific action
            int position = undoStack.search(new Action("FORMAT", "Made text bold"));
            if (position > 0) {
                System.out.println("FORMAT action is " + position + " from top");
            }
            
            System.out.println("Remaining actions: " + undoStack.size());  // 2
        }
    }
}
```

### Example 7: Session Management

```java
import com.kuber.fastcollection.*;
import java.io.Serializable;
import java.util.*;

public class SessionManagerExample {
    
    public static class Session implements Serializable {
        private static final long serialVersionUID = 1L;
        public String sessionId;
        public String userId;
        public Map<String, Object> attributes;
        public long createdAt;
        public long lastAccessedAt;
        
        public Session(String sessionId, String userId) {
            this.sessionId = sessionId;
            this.userId = userId;
            this.attributes = new HashMap<>();
            this.createdAt = System.currentTimeMillis();
            this.lastAccessedAt = this.createdAt;
        }
        
        public void setAttribute(String key, Object value) {
            attributes.put(key, value);
            lastAccessedAt = System.currentTimeMillis();
        }
        
        public Object getAttribute(String key) {
            lastAccessedAt = System.currentTimeMillis();
            return attributes.get(key);
        }
    }
    
    private FastCollectionMap<String, Session> sessions;
    private static final int SESSION_TTL = 30 * 60;  // 30 minutes
    
    public SessionManagerExample(String storagePath) {
        sessions = new FastCollectionMap<>(storagePath);
    }
    
    public Session createSession(String userId) {
        String sessionId = UUID.randomUUID().toString();
        Session session = new Session(sessionId, userId);
        sessions.put(sessionId, session, SESSION_TTL);
        return session;
    }
    
    public Session getSession(String sessionId) {
        Session session = sessions.get(sessionId);
        if (session != null) {
            // Refresh TTL on access
            sessions.setTTL(sessionId, SESSION_TTL);
        }
        return session;
    }
    
    public void invalidateSession(String sessionId) {
        sessions.remove(sessionId);
    }
    
    public int getActiveSessionCount() {
        return sessions.size();
    }
    
    public void close() {
        sessions.close();
    }
    
    public static void main(String[] args) {
        SessionManagerExample manager = new SessionManagerExample("/tmp/sessions.fc");
        
        // Create sessions
        Session s1 = manager.createSession("user_001");
        Session s2 = manager.createSession("user_002");
        
        // Store session data
        s1.setAttribute("cart", Arrays.asList("item1", "item2"));
        s1.setAttribute("preferences", Map.of("theme", "dark"));
        
        // Retrieve session
        Session retrieved = manager.getSession(s1.sessionId);
        if (retrieved != null) {
            System.out.println("User: " + retrieved.userId);
            System.out.println("Cart: " + retrieved.getAttribute("cart"));
        }
        
        System.out.println("Active sessions: " + manager.getActiveSessionCount());
        
        // Invalidate session
        manager.invalidateSession(s2.sessionId);
        
        manager.close();
    }
}
```

### Example 8: Rate Limiter

```java
import com.kuber.fastcollection.*;
import java.io.Serializable;

public class RateLimiterExample {
    
    public static class RateLimitEntry implements Serializable {
        private static final long serialVersionUID = 1L;
        public int count;
        public long windowStart;
        
        public RateLimitEntry() {
            this.count = 1;
            this.windowStart = System.currentTimeMillis();
        }
        
        public void increment() {
            this.count++;
        }
    }
    
    private FastCollectionMap<String, RateLimitEntry> limiter;
    private int maxRequests;
    private int windowSeconds;
    
    public RateLimiterExample(String storagePath, int maxRequests, int windowSeconds) {
        this.limiter = new FastCollectionMap<>(storagePath);
        this.maxRequests = maxRequests;
        this.windowSeconds = windowSeconds;
    }
    
    public boolean allowRequest(String clientId) {
        RateLimitEntry entry = limiter.get(clientId);
        
        if (entry == null) {
            // First request in window
            limiter.put(clientId, new RateLimitEntry(), windowSeconds);
            return true;
        }
        
        if (entry.count >= maxRequests) {
            // Rate limit exceeded
            return false;
        }
        
        // Increment count
        entry.increment();
        limiter.put(clientId, entry, windowSeconds);
        return true;
    }
    
    public int getRemainingRequests(String clientId) {
        RateLimitEntry entry = limiter.get(clientId);
        if (entry == null) {
            return maxRequests;
        }
        return Math.max(0, maxRequests - entry.count);
    }
    
    public void close() {
        limiter.close();
    }
    
    public static void main(String[] args) throws InterruptedException {
        // 100 requests per minute
        RateLimiterExample rateLimiter = new RateLimiterExample(
            "/tmp/ratelimit.fc", 100, 60
        );
        
        String clientId = "api_client_001";
        
        // Simulate requests
        for (int i = 0; i < 105; i++) {
            if (rateLimiter.allowRequest(clientId)) {
                System.out.println("Request " + (i + 1) + ": ALLOWED");
            } else {
                System.out.println("Request " + (i + 1) + ": RATE LIMITED");
            }
        }
        
        System.out.println("Remaining: " + rateLimiter.getRemainingRequests(clientId));
        
        rateLimiter.close();
    }
}
```

### Example 9: Distributed Counter

```java
import com.kuber.fastcollection.*;
import java.io.Serializable;
import java.util.concurrent.*;

public class DistributedCounterExample {
    
    public static class Counter implements Serializable {
        private static final long serialVersionUID = 1L;
        public long value;
        
        public Counter(long value) {
            this.value = value;
        }
    }
    
    private FastCollectionMap<String, Counter> counters;
    
    public DistributedCounterExample(String storagePath) {
        this.counters = new FastCollectionMap<>(storagePath);
    }
    
    public synchronized long increment(String name) {
        Counter counter = counters.get(name);
        if (counter == null) {
            counter = new Counter(0);
        }
        counter.value++;
        counters.put(name, counter);
        return counter.value;
    }
    
    public synchronized long decrement(String name) {
        Counter counter = counters.get(name);
        if (counter == null) {
            counter = new Counter(0);
        }
        counter.value--;
        counters.put(name, counter);
        return counter.value;
    }
    
    public long get(String name) {
        Counter counter = counters.get(name);
        return counter != null ? counter.value : 0;
    }
    
    public void close() {
        counters.close();
    }
    
    public static void main(String[] args) throws Exception {
        DistributedCounterExample counter = new DistributedCounterExample("/tmp/counters.fc");
        
        // Simulate concurrent increments
        ExecutorService executor = Executors.newFixedThreadPool(10);
        
        for (int i = 0; i < 1000; i++) {
            executor.submit(() -> counter.increment("page_views"));
        }
        
        executor.shutdown();
        executor.awaitTermination(10, TimeUnit.SECONDS);
        
        System.out.println("Total page views: " + counter.get("page_views"));
        
        counter.close();
    }
}
```

### Example 10: Event Log with Retention

```java
import com.kuber.fastcollection.*;
import java.io.Serializable;
import java.time.*;
import java.time.format.DateTimeFormatter;

public class EventLogExample {
    
    public static class LogEvent implements Serializable {
        private static final long serialVersionUID = 1L;
        public String level;     // INFO, WARN, ERROR
        public String message;
        public String source;
        public long timestamp;
        
        public LogEvent(String level, String message, String source) {
            this.level = level;
            this.message = message;
            this.source = source;
            this.timestamp = System.currentTimeMillis();
        }
        
        @Override
        public String toString() {
            String time = Instant.ofEpochMilli(timestamp)
                .atZone(ZoneId.systemDefault())
                .format(DateTimeFormatter.ISO_LOCAL_DATE_TIME);
            return String.format("[%s] %s - %s: %s", time, level, source, message);
        }
    }
    
    private FastCollectionList<LogEvent> logs;
    private int retentionSeconds;
    
    public EventLogExample(String storagePath, int retentionDays) {
        this.logs = new FastCollectionList<>(storagePath, LogEvent.class);
        this.retentionSeconds = retentionDays * 24 * 60 * 60;
    }
    
    public void info(String source, String message) {
        log("INFO", source, message);
    }
    
    public void warn(String source, String message) {
        log("WARN", source, message);
    }
    
    public void error(String source, String message) {
        log("ERROR", source, message);
    }
    
    private void log(String level, String source, String message) {
        LogEvent event = new LogEvent(level, message, source);
        logs.add(event, retentionSeconds);
    }
    
    public void printLast(int count) {
        int size = logs.size();
        int start = Math.max(0, size - count);
        
        System.out.println("=== Last " + count + " log entries ===");
        for (int i = start; i < size; i++) {
            LogEvent event = logs.get(i);
            if (event != null) {
                System.out.println(event);
            }
        }
    }
    
    public int cleanup() {
        return logs.removeExpired();
    }
    
    public void close() {
        logs.close();
    }
    
    public static void main(String[] args) {
        // Keep logs for 7 days
        EventLogExample logger = new EventLogExample("/tmp/events.fc", 7);
        
        // Log some events
        logger.info("WebServer", "Server started on port 8080");
        logger.info("Database", "Connected to PostgreSQL");
        logger.warn("WebServer", "High memory usage detected");
        logger.error("PaymentService", "Payment gateway timeout");
        logger.info("UserService", "User 'john' logged in");
        
        // Print recent logs
        logger.printLast(10);
        
        // Cleanup old entries
        int removed = logger.cleanup();
        System.out.println("Cleaned up " + removed + " old entries");
        
        logger.close();
    }
}
```

---

## Python Examples

### Example 1: Basic List Operations

```python
from fastcollection import FastList

# Create a persistent list
with FastList("/tmp/mylist.fc") as lst:
    # Add elements
    lst.add(b"Hello")
    lst.add(b"World")
    lst.add(b"FastCollection")
    
    # Access elements
    print(f"First: {lst.get(0).decode()}")  # Hello
    print(f"Size: {lst.size()}")            # 3
    
    # Iterate
    for i in range(lst.size()):
        item = lst.get(i)
        if item:
            print(f"Item {i}: {item.decode()}")
    
    # Remove
    lst.remove(1)  # Remove "World"
    
    print(f"After removal: {lst.size()}")  # 2
```

### Example 2: List with TTL

```python
from fastcollection import FastList
import time

with FastList("/tmp/cache.fc") as cache:
    # Add items with different TTLs
    cache.add(b"permanent", ttl=-1)     # Never expires
    cache.add(b"5-minutes", ttl=300)    # 5 minutes
    cache.add(b"10-seconds", ttl=10)    # 10 seconds
    
    print(f"Initial size: {cache.size()}")  # 3
    
    # Check TTL
    print(f"TTL[0]: {cache.get_ttl(0)}")  # -1
    print(f"TTL[1]: {cache.get_ttl(1)}")  # ~300
    print(f"TTL[2]: {cache.get_ttl(2)}")  # ~10
    
    # Wait for expiry
    time.sleep(12)
    
    print(f"After expiry: {cache.size()}")  # 2
    
    # Update TTL
    cache.set_ttl(1, 600)  # Extend to 10 minutes
```

### Example 3: Key-Value Cache

```python
from fastcollection import FastMap

with FastMap("/tmp/cache.fc") as cache:
    # Simple put/get
    cache.put(b"user:1001", b"John Doe")
    cache.put(b"user:1002", b"Jane Smith")
    
    user = cache.get(b"user:1001")
    print(f"User 1001: {user.decode()}")  # John Doe
    
    # Put with TTL (30 minutes)
    cache.put(b"session:abc", b"session_data", ttl=1800)
    
    # Check existence
    if cache.contains_key(b"user:1002"):
        print("User 1002 exists!")
    
    # Get TTL
    ttl = cache.get_ttl(b"session:abc")
    print(f"Session TTL: {ttl} seconds")
    
    # Remove
    cache.remove(b"user:1002")
    
    print(f"Cache size: {cache.size()}")
```

### Example 4: Unique Visitor Tracking

```python
from fastcollection import FastSet

with FastSet("/tmp/visitors.fc") as visitors:
    TTL_24H = 24 * 60 * 60  # 24 hours
    
    # Track visitors
    visitors.add(b"visitor_001", ttl=TTL_24H)
    visitors.add(b"visitor_002", ttl=TTL_24H)
    visitors.add(b"visitor_003", ttl=TTL_24H)
    
    # Check if seen before
    if visitors.contains(b"visitor_001"):
        print("Welcome back!")
    
    # Add new visitors
    new_visitors = [b"visitor_004", b"visitor_005"]
    for v in new_visitors:
        if visitors.add(v, ttl=TTL_24H):
            print(f"New visitor: {v.decode()}")
    
    print(f"Unique visitors: {visitors.size()}")
```

### Example 5: Task Queue

```python
from fastcollection import FastQueue
import json

with FastQueue("/tmp/tasks.fc") as queue:
    # Add tasks (JSON serialized)
    tasks = [
        {"id": "t1", "type": "email", "priority": 1},
        {"id": "t2", "type": "report", "priority": 2},
        {"id": "t3", "type": "backup", "priority": 3},
    ]
    
    for task in tasks:
        queue.offer(json.dumps(task).encode(), ttl=3600)
    
    # Add urgent task to front
    urgent = {"id": "t0", "type": "alert", "priority": 0}
    queue.offer_first(json.dumps(urgent).encode(), ttl=3600)
    
    print(f"Queue size: {queue.size()}")  # 4
    
    # Process tasks
    while True:
        data = queue.poll()
        if data is None:
            break
        task = json.loads(data.decode())
        print(f"Processing: {task}")
```

### Example 6: Undo Stack

```python
from fastcollection import FastStack
import json
import time

with FastStack("/tmp/undo.fc") as undo:
    # Perform actions
    actions = [
        {"type": "INSERT", "text": "Hello"},
        {"type": "FORMAT", "style": "bold"},
        {"type": "INSERT", "text": "World"},
        {"type": "DELETE", "chars": 1},
    ]
    
    for action in actions:
        action["timestamp"] = time.time()
        undo.push(json.dumps(action).encode())
    
    print(f"Actions in stack: {undo.size()}")  # 4
    
    # Peek at last action
    last = json.loads(undo.peek().decode())
    print(f"Last action: {last['type']}")  # DELETE
    
    # Undo last 2 actions
    print("\nUndoing:")
    for _ in range(2):
        data = undo.pop()
        if data:
            action = json.loads(data.decode())
            print(f"  Undone: {action['type']}")
    
    print(f"Remaining: {undo.size()}")  # 2
```

### Example 7: Rate Limiter

```python
from fastcollection import FastMap
import json
import time

class RateLimiter:
    def __init__(self, path: str, max_requests: int, window_seconds: int):
        self.store = FastMap(path)
        self.max_requests = max_requests
        self.window_seconds = window_seconds
    
    def allow_request(self, client_id: str) -> bool:
        key = client_id.encode()
        data = self.store.get(key)
        
        if data is None:
            # First request
            entry = {"count": 1, "start": time.time()}
            self.store.put(key, json.dumps(entry).encode(), ttl=self.window_seconds)
            return True
        
        entry = json.loads(data.decode())
        
        if entry["count"] >= self.max_requests:
            return False
        
        entry["count"] += 1
        self.store.put(key, json.dumps(entry).encode(), ttl=self.window_seconds)
        return True
    
    def get_remaining(self, client_id: str) -> int:
        key = client_id.encode()
        data = self.store.get(key)
        
        if data is None:
            return self.max_requests
        
        entry = json.loads(data.decode())
        return max(0, self.max_requests - entry["count"])
    
    def close(self):
        self.store.close()


# Usage
limiter = RateLimiter("/tmp/ratelimit.fc", max_requests=100, window_seconds=60)

for i in range(105):
    if limiter.allow_request("client_001"):
        print(f"Request {i + 1}: ALLOWED")
    else:
        print(f"Request {i + 1}: RATE LIMITED")

print(f"Remaining: {limiter.get_remaining('client_001')}")
limiter.close()
```

### Example 8: Event Logger

```python
from fastcollection import FastList
import json
import time
from datetime import datetime

class EventLogger:
    def __init__(self, path: str, retention_days: int = 7):
        self.logs = FastList(path)
        self.retention_seconds = retention_days * 24 * 60 * 60
    
    def log(self, level: str, source: str, message: str):
        event = {
            "level": level,
            "source": source,
            "message": message,
            "timestamp": time.time()
        }
        self.logs.add(json.dumps(event).encode(), ttl=self.retention_seconds)
    
    def info(self, source: str, message: str):
        self.log("INFO", source, message)
    
    def warn(self, source: str, message: str):
        self.log("WARN", source, message)
    
    def error(self, source: str, message: str):
        self.log("ERROR", source, message)
    
    def print_last(self, count: int = 10):
        size = self.logs.size()
        start = max(0, size - count)
        
        print(f"=== Last {count} log entries ===")
        for i in range(start, size):
            data = self.logs.get(i)
            if data:
                event = json.loads(data.decode())
                ts = datetime.fromtimestamp(event["timestamp"]).isoformat()
                print(f"[{ts}] {event['level']} - {event['source']}: {event['message']}")
    
    def close(self):
        self.logs.close()


# Usage
logger = EventLogger("/tmp/events.fc", retention_days=7)

logger.info("WebServer", "Server started on port 8080")
logger.info("Database", "Connected to PostgreSQL")
logger.warn("WebServer", "High memory usage")
logger.error("PaymentService", "Gateway timeout")

logger.print_last(10)
logger.close()
```

---

## C++ Examples

### Example 1: Basic List Operations

```cpp
#include "fastcollection.h"
#include <iostream>
#include <string>

int main() {
    using namespace fastcollection;
    
    // Create a persistent list
    FastList list("/tmp/mylist.fc", 64 * 1024 * 1024, true);
    
    // Add elements
    std::string items[] = {"Hello", "World", "FastCollection"};
    for (const auto& item : items) {
        list.add(reinterpret_cast<const uint8_t*>(item.data()), item.size());
    }
    
    std::cout << "Size: " << list.size() << std::endl;  // 3
    
    // Get first element
    std::vector<uint8_t> result;
    if (list.get(0, result)) {
        std::string value(result.begin(), result.end());
        std::cout << "First: " << value << std::endl;  // Hello
    }
    
    // Iterate
    for (size_t i = 0; i < list.size(); i++) {
        if (list.get(i, result)) {
            std::string value(result.begin(), result.end());
            std::cout << "Item " << i << ": " << value << std::endl;
        }
    }
    
    // Remove
    list.remove(1);  // Remove "World"
    
    std::cout << "After removal: " << list.size() << std::endl;  // 2
    
    return 0;
}
```

### Example 2: List with TTL

```cpp
#include "fastcollection.h"
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    using namespace fastcollection;
    
    FastList cache("/tmp/cache.fc", 64 * 1024 * 1024, true);
    
    // Add with different TTLs
    std::string permanent = "permanent";
    std::string fiveMin = "5-minutes";
    std::string tenSec = "10-seconds";
    
    cache.add(reinterpret_cast<const uint8_t*>(permanent.data()), permanent.size(), -1);
    cache.add(reinterpret_cast<const uint8_t*>(fiveMin.data()), fiveMin.size(), 300);
    cache.add(reinterpret_cast<const uint8_t*>(tenSec.data()), tenSec.size(), 10);
    
    std::cout << "Initial size: " << cache.size() << std::endl;  // 3
    
    // Check TTLs
    std::cout << "TTL[0]: " << cache.getTTL(0) << std::endl;  // -1
    std::cout << "TTL[1]: " << cache.getTTL(1) << std::endl;  // ~300
    std::cout << "TTL[2]: " << cache.getTTL(2) << std::endl;  // ~10
    
    // Wait for expiry
    std::this_thread::sleep_for(std::chrono::seconds(12));
    
    std::cout << "After expiry: " << cache.size() << std::endl;  // 2
    
    // Update TTL
    cache.setTTL(1, 600);  // Extend to 10 minutes
    
    return 0;
}
```

### Example 3: Key-Value Map

```cpp
#include "fastcollection.h"
#include <iostream>
#include <string>

int main() {
    using namespace fastcollection;
    
    FastMap cache("/tmp/cache.fc", 64 * 1024 * 1024, true);
    
    // Put key-value pairs
    std::string key1 = "user:1001", val1 = "John Doe";
    std::string key2 = "user:1002", val2 = "Jane Smith";
    
    cache.put(
        reinterpret_cast<const uint8_t*>(key1.data()), key1.size(),
        reinterpret_cast<const uint8_t*>(val1.data()), val1.size()
    );
    
    cache.put(
        reinterpret_cast<const uint8_t*>(key2.data()), key2.size(),
        reinterpret_cast<const uint8_t*>(val2.data()), val2.size()
    );
    
    // Get value
    std::vector<uint8_t> result;
    if (cache.get(reinterpret_cast<const uint8_t*>(key1.data()), key1.size(), result)) {
        std::string value(result.begin(), result.end());
        std::cout << "User 1001: " << value << std::endl;  // John Doe
    }
    
    // Put with TTL (30 minutes)
    std::string sessionKey = "session:abc";
    std::string sessionData = "session_data";
    cache.put(
        reinterpret_cast<const uint8_t*>(sessionKey.data()), sessionKey.size(),
        reinterpret_cast<const uint8_t*>(sessionData.data()), sessionData.size(),
        1800
    );
    
    // Check TTL
    int64_t ttl = cache.getTTL(
        reinterpret_cast<const uint8_t*>(sessionKey.data()), sessionKey.size()
    );
    std::cout << "Session TTL: " << ttl << " seconds" << std::endl;
    
    // Check existence
    bool exists = cache.containsKey(
        reinterpret_cast<const uint8_t*>(key2.data()), key2.size()
    );
    std::cout << "User 1002 exists: " << (exists ? "yes" : "no") << std::endl;
    
    // Remove
    cache.remove(reinterpret_cast<const uint8_t*>(key2.data()), key2.size());
    
    std::cout << "Cache size: " << cache.size() << std::endl;
    
    return 0;
}
```

### Example 4: Set for Unique Values

```cpp
#include "fastcollection.h"
#include <iostream>
#include <string>
#include <vector>

int main() {
    using namespace fastcollection;
    
    FastSet visitors("/tmp/visitors.fc", 64 * 1024 * 1024, true);
    
    const int TTL_24H = 24 * 60 * 60;
    
    // Track visitors
    std::vector<std::string> ids = {"visitor_001", "visitor_002", "visitor_003"};
    for (const auto& id : ids) {
        visitors.add(reinterpret_cast<const uint8_t*>(id.data()), id.size(), TTL_24H);
    }
    
    // Check if seen before
    std::string checkId = "visitor_001";
    if (visitors.contains(reinterpret_cast<const uint8_t*>(checkId.data()), checkId.size())) {
        std::cout << "Welcome back, " << checkId << "!" << std::endl;
    }
    
    // Add new visitors
    std::vector<std::string> newIds = {"visitor_004", "visitor_005"};
    for (const auto& id : newIds) {
        if (visitors.add(reinterpret_cast<const uint8_t*>(id.data()), id.size(), TTL_24H)) {
            std::cout << "New visitor: " << id << std::endl;
        }
    }
    
    std::cout << "Unique visitors: " << visitors.size() << std::endl;
    
    return 0;
}
```

### Example 5: Task Queue

```cpp
#include "fastcollection.h"
#include <iostream>
#include <string>
#include <sstream>

struct Task {
    std::string id;
    std::string type;
    int priority;
    
    std::string serialize() const {
        std::ostringstream oss;
        oss << id << "|" << type << "|" << priority;
        return oss.str();
    }
    
    static Task deserialize(const std::string& data) {
        Task task;
        std::istringstream iss(data);
        std::getline(iss, task.id, '|');
        std::getline(iss, task.type, '|');
        iss >> task.priority;
        return task;
    }
};

int main() {
    using namespace fastcollection;
    
    FastQueue queue("/tmp/tasks.fc", 64 * 1024 * 1024, true);
    
    const int TASK_TTL = 3600;  // 1 hour
    
    // Add tasks
    std::vector<Task> tasks = {
        {"t1", "email", 1},
        {"t2", "report", 2},
        {"t3", "backup", 3}
    };
    
    for (const auto& task : tasks) {
        std::string data = task.serialize();
        queue.offer(reinterpret_cast<const uint8_t*>(data.data()), data.size(), TASK_TTL);
    }
    
    // Add urgent task to front
    Task urgent{"t0", "alert", 0};
    std::string urgentData = urgent.serialize();
    queue.offerFirst(reinterpret_cast<const uint8_t*>(urgentData.data()), urgentData.size(), TASK_TTL);
    
    std::cout << "Queue size: " << queue.size() << std::endl;  // 4
    
    // Process tasks
    std::vector<uint8_t> result;
    while (queue.poll(result)) {
        std::string data(result.begin(), result.end());
        Task task = Task::deserialize(data);
        std::cout << "Processing: " << task.id << " (" << task.type << ")" << std::endl;
        result.clear();
    }
    
    return 0;
}
```

### Example 6: Undo Stack

```cpp
#include "fastcollection.h"
#include <iostream>
#include <string>
#include <sstream>
#include <ctime>

struct Action {
    std::string type;
    std::string description;
    time_t timestamp;
    
    std::string serialize() const {
        std::ostringstream oss;
        oss << type << "|" << description << "|" << timestamp;
        return oss.str();
    }
    
    static Action deserialize(const std::string& data) {
        Action action;
        std::istringstream iss(data);
        std::getline(iss, action.type, '|');
        std::getline(iss, action.description, '|');
        iss >> action.timestamp;
        return action;
    }
};

int main() {
    using namespace fastcollection;
    
    FastStack undo("/tmp/undo.fc", 64 * 1024 * 1024, true);
    
    // Perform actions
    std::vector<Action> actions = {
        {"INSERT", "Added text 'Hello'", time(nullptr)},
        {"FORMAT", "Made text bold", time(nullptr)},
        {"INSERT", "Added text 'World'", time(nullptr)},
        {"DELETE", "Removed character", time(nullptr)}
    };
    
    for (const auto& action : actions) {
        std::string data = action.serialize();
        undo.push(reinterpret_cast<const uint8_t*>(data.data()), data.size());
    }
    
    std::cout << "Actions in stack: " << undo.size() << std::endl;  // 4
    
    // Peek at last action
    std::vector<uint8_t> result;
    if (undo.peek(result)) {
        std::string data(result.begin(), result.end());
        Action last = Action::deserialize(data);
        std::cout << "Last action: " << last.type << std::endl;  // DELETE
    }
    
    // Undo last 2 actions
    std::cout << "\nUndoing:" << std::endl;
    for (int i = 0; i < 2; i++) {
        result.clear();
        if (undo.pop(result)) {
            std::string data(result.begin(), result.end());
            Action action = Action::deserialize(data);
            std::cout << "  Undone: " << action.type << std::endl;
        }
    }
    
    std::cout << "Remaining: " << undo.size() << std::endl;  // 2
    
    return 0;
}
```

---

## Common Patterns

### Pattern 1: Safe Resource Management

**Java:**
```java
// Always use try-with-resources
try (FastCollectionList<String> list = new FastCollectionList<>(path, String.class)) {
    // Use list...
}  // Auto-closes

// Or explicit close
FastCollectionList<String> list = new FastCollectionList<>(path, String.class);
try {
    // Use list...
} finally {
    list.close();
}
```

**Python:**
```python
# Use context manager
with FastList("/tmp/list.fc") as lst:
    # Use list...

# Or explicit close
lst = FastList("/tmp/list.fc")
try:
    # Use list...
finally:
    lst.close()
```

**C++:**
```cpp
// Stack allocation (RAII)
{
    FastList list("/tmp/list.fc", 64*1024*1024, true);
    // Use list...
}  // Destructor closes

// Or explicit destruction
FastList* list = new FastList("/tmp/list.fc", 64*1024*1024, true);
// Use list...
delete list;
```

### Pattern 2: Atomic Operations

```java
// Java: Thread-safe cache update
FastCollectionMap<String, Integer> counters = new FastCollectionMap<>(path);

synchronized(counters) {
    Integer count = counters.get("visits");
    count = (count == null) ? 1 : count + 1;
    counters.put("visits", count);
}
```

### Pattern 3: TTL-Based Caching

```java
// Java: Multi-tier TTL strategy
public void cacheWithTier(String key, String value, String tier) {
    int ttl;
    switch (tier) {
        case "hot":   ttl = 60;     break;  // 1 minute
        case "warm":  ttl = 300;    break;  // 5 minutes
        case "cold":  ttl = 3600;   break;  // 1 hour
        default:      ttl = -1;     break;  // Never expires
    }
    cache.put(key, value, ttl);
}
```

---

## Best Practices

### 1. File Path Management

```java
// Use dedicated directories
String baseDir = System.getProperty("user.home") + "/.fastcollection";
new File(baseDir).mkdirs();

String listPath = baseDir + "/mylist.fc";
String mapPath = baseDir + "/mymap.fc";
```

### 2. Error Handling

```java
try {
    FastCollectionList<String> list = new FastCollectionList<>(path, String.class);
    // Use list...
} catch (FastCollectionException e) {
    logger.error("FastCollection error: " + e.getMessage());
    // Handle or rethrow
}
```

### 3. TTL Strategy

```java
// Define TTL constants
public class TTL {
    public static final int NEVER = -1;
    public static final int MINUTE = 60;
    public static final int HOUR = 3600;
    public static final int DAY = 86400;
    public static final int WEEK = 604800;
}

// Use consistently
cache.put("session", data, TTL.HOUR);
cache.put("user", data, TTL.DAY);
cache.put("config", data, TTL.NEVER);
```

### 4. Serialization

```java
// For complex objects, use JSON
import com.fasterxml.jackson.databind.ObjectMapper;

ObjectMapper mapper = new ObjectMapper();

// Serialize
String json = mapper.writeValueAsString(myObject);
list.add(json);

// Deserialize
String json = list.get(0);
MyObject obj = mapper.readValue(json, MyObject.class);
```

### 5. Monitoring

```java
// Periodic cleanup and stats
ScheduledExecutorService scheduler = Executors.newScheduledThreadPool(1);
scheduler.scheduleAtFixedRate(() -> {
    int removed = cache.removeExpired();
    logger.info("Cleaned {} expired entries, size: {}", removed, cache.size());
}, 1, 1, TimeUnit.MINUTES);
```

---

## Advanced Integration Examples

### Spring Boot Integration (Java)

```java
import com.kuber.fastcollection.*;
import org.springframework.stereotype.Service;
import org.springframework.beans.factory.annotation.Value;
import javax.annotation.PostConstruct;
import javax.annotation.PreDestroy;

@Service
public class CacheService {
    
    @Value("${fastcollection.path:/var/cache/myapp}")
    private String cachePath;
    
    private FastCollectionMap<String, Object> cache;
    
    @PostConstruct
    public void init() {
        cache = new FastCollectionMap<>(cachePath + "/cache.fc", 256 * 1024 * 1024, true);
    }
    
    @PreDestroy
    public void destroy() {
        if (cache != null) {
            cache.close();
        }
    }
    
    public <T> void put(String key, T value, int ttlSeconds) {
        cache.put(key, value, ttlSeconds);
    }
    
    @SuppressWarnings("unchecked")
    public <T> T get(String key) {
        return (T) cache.get(key);
    }
    
    public void evict(String key) {
        cache.remove(key);
    }
    
    public int cleanup() {
        return cache.removeExpired();
    }
}

// Usage in Controller
@RestController
@RequestMapping("/api/users")
public class UserController {
    
    @Autowired
    private CacheService cache;
    
    @Autowired
    private UserRepository userRepo;
    
    @GetMapping("/{id}")
    public User getUser(@PathVariable Long id) {
        String cacheKey = "user:" + id;
        
        // Try cache first
        User user = cache.get(cacheKey);
        if (user != null) {
            return user;
        }
        
        // Load from database
        user = userRepo.findById(id).orElseThrow();
        
        // Cache for 5 minutes
        cache.put(cacheKey, user, 300);
        
        return user;
    }
}
```

### Flask Integration (Python)

```python
from fastcollection import FastMap
from flask import Flask, jsonify, request
from functools import wraps
import json

app = Flask(__name__)

# Initialize FastCollection
cache = FastMap("/var/cache/myapp/cache.fc")
sessions = FastMap("/var/cache/myapp/sessions.fc")

def cached(ttl_seconds=300):
    """Decorator for caching endpoint responses"""
    def decorator(f):
        @wraps(f)
        def wrapper(*args, **kwargs):
            cache_key = f"{f.__name__}:{request.path}:{request.query_string.decode()}"
            
            # Check cache
            cached_data = cache.get(cache_key.encode())
            if cached_data:
                return jsonify(json.loads(cached_data.decode()))
            
            # Execute function
            result = f(*args, **kwargs)
            
            # Cache result
            cache.put(cache_key.encode(), json.dumps(result).encode(), ttl=ttl_seconds)
            
            return jsonify(result)
        return wrapper
    return decorator

@app.route('/api/products')
@cached(ttl_seconds=60)
def get_products():
    # Expensive database query
    products = Product.query.all()
    return [p.to_dict() for p in products]

@app.route('/api/users/<int:user_id>')
@cached(ttl_seconds=300)
def get_user(user_id):
    user = User.query.get_or_404(user_id)
    return user.to_dict()

# Session middleware
@app.before_request
def load_session():
    session_id = request.cookies.get('session_id')
    if session_id:
        data = sessions.get(session_id.encode())
        if data:
            request.session = json.loads(data.decode())
            # Refresh TTL
            sessions.put(session_id.encode(), data, ttl=3600)
        else:
            request.session = {}
    else:
        request.session = {}

@app.teardown_appcontext
def cleanup(exception):
    # Cleanup is automatic with TTL
    pass
```

### Microservices Message Queue (Java)

```java
import com.kuber.fastcollection.*;
import java.io.Serializable;
import java.util.*;
import java.util.concurrent.*;

/**
 * Simple pub/sub messaging system using FastCollection
 */
public class MessageBroker {
    
    public static class Message implements Serializable {
        private static final long serialVersionUID = 1L;
        public String id;
        public String topic;
        public String payload;
        public long timestamp;
        public Map<String, String> headers;
        
        public Message(String topic, String payload) {
            this.id = UUID.randomUUID().toString();
            this.topic = topic;
            this.payload = payload;
            this.timestamp = System.currentTimeMillis();
            this.headers = new HashMap<>();
        }
    }
    
    public interface MessageHandler {
        void handle(Message message);
    }
    
    private final Map<String, FastCollectionQueue<Message>> topicQueues = new ConcurrentHashMap<>();
    private final Map<String, List<MessageHandler>> subscribers = new ConcurrentHashMap<>();
    private final String basePath;
    private final ExecutorService executor;
    private volatile boolean running = true;
    
    public MessageBroker(String basePath) {
        this.basePath = basePath;
        this.executor = Executors.newCachedThreadPool();
    }
    
    private FastCollectionQueue<Message> getQueue(String topic) {
        return topicQueues.computeIfAbsent(topic, t -> 
            new FastCollectionQueue<>(basePath + "/" + t + ".fc", 64 * 1024 * 1024, true)
        );
    }
    
    public void publish(String topic, String payload) {
        Message message = new Message(topic, payload);
        getQueue(topic).offer(message, 3600);  // 1 hour TTL
    }
    
    public void publish(String topic, String payload, Map<String, String> headers) {
        Message message = new Message(topic, payload);
        message.headers.putAll(headers);
        getQueue(topic).offer(message, 3600);
    }
    
    public void subscribe(String topic, MessageHandler handler) {
        subscribers.computeIfAbsent(topic, t -> new CopyOnWriteArrayList<>()).add(handler);
        
        // Start consumer thread for this topic
        executor.submit(() -> {
            FastCollectionQueue<Message> queue = getQueue(topic);
            
            while (running) {
                try {
                    Message message = queue.poll();
                    if (message != null) {
                        List<MessageHandler> handlers = subscribers.get(topic);
                        if (handlers != null) {
                            for (MessageHandler h : handlers) {
                                try {
                                    h.handle(message);
                                } catch (Exception e) {
                                    System.err.println("Handler error: " + e.getMessage());
                                }
                            }
                        }
                    } else {
                        Thread.sleep(100);  // Polling interval
                    }
                } catch (InterruptedException e) {
                    break;
                }
            }
        });
    }
    
    public void shutdown() {
        running = false;
        executor.shutdown();
        topicQueues.values().forEach(FastCollectionQueue::close);
    }
    
    // Usage example
    public static void main(String[] args) throws InterruptedException {
        MessageBroker broker = new MessageBroker("/tmp/messagebroker");
        
        // Subscribe to topics
        broker.subscribe("orders", msg -> {
            System.out.println("Order received: " + msg.payload);
        });
        
        broker.subscribe("notifications", msg -> {
            System.out.println("Notification: " + msg.payload);
        });
        
        // Publish messages
        broker.publish("orders", "{\"orderId\": 12345, \"amount\": 99.99}");
        broker.publish("notifications", "New order placed!");
        
        Thread.sleep(1000);
        broker.shutdown();
    }
}
```

### Real-Time Analytics Counter (Python)

```python
from fastcollection import FastMap, FastList
import json
import time
from datetime import datetime, timedelta
from typing import Dict, Any, List
from dataclasses import dataclass, asdict
import threading

@dataclass
class MetricPoint:
    timestamp: float
    value: float
    tags: Dict[str, str]
    
    def to_json(self) -> bytes:
        return json.dumps(asdict(self)).encode()
    
    @classmethod
    def from_json(cls, data: bytes) -> 'MetricPoint':
        return cls(**json.loads(data.decode()))


class RealTimeAnalytics:
    """
    Real-time analytics with time-series data and counters
    """
    
    def __init__(self, base_path: str):
        self.counters = FastMap(f"{base_path}/counters.fc")
        self.time_series = FastMap(f"{base_path}/timeseries.fc")
        self._lock = threading.Lock()
    
    def increment(self, metric: str, value: int = 1, ttl: int = 86400):
        """Increment a counter"""
        with self._lock:
            key = metric.encode()
            current = self.counters.get(key)
            
            if current:
                count = int(current.decode()) + value
            else:
                count = value
            
            self.counters.put(key, str(count).encode(), ttl=ttl)
    
    def get_counter(self, metric: str) -> int:
        """Get current counter value"""
        data = self.counters.get(metric.encode())
        return int(data.decode()) if data else 0
    
    def record_metric(self, name: str, value: float, tags: Dict[str, str] = None, ttl: int = 86400):
        """Record a time-series metric point"""
        point = MetricPoint(
            timestamp=time.time(),
            value=value,
            tags=tags or {}
        )
        
        # Key format: metric:YYYYMMDD:HHMM
        now = datetime.now()
        time_bucket = now.strftime("%Y%m%d:%H%M")
        key = f"{name}:{time_bucket}".encode()
        
        # Get existing points for this bucket
        data = self.time_series.get(key)
        if data:
            points = json.loads(data.decode())
        else:
            points = []
        
        points.append(asdict(point))
        self.time_series.put(key, json.dumps(points).encode(), ttl=ttl)
    
    def get_metrics(self, name: str, minutes: int = 60) -> List[MetricPoint]:
        """Get metrics for the last N minutes"""
        result = []
        now = datetime.now()
        
        for i in range(minutes):
            t = now - timedelta(minutes=i)
            time_bucket = t.strftime("%Y%m%d:%H%M")
            key = f"{name}:{time_bucket}".encode()
            
            data = self.time_series.get(key)
            if data:
                points = json.loads(data.decode())
                result.extend([MetricPoint(**p) for p in points])
        
        return sorted(result, key=lambda p: p.timestamp)
    
    def get_average(self, name: str, minutes: int = 60) -> float:
        """Get average metric value over time period"""
        points = self.get_metrics(name, minutes)
        if not points:
            return 0.0
        return sum(p.value for p in points) / len(points)
    
    def close(self):
        self.counters.close()
        self.time_series.close()


# Usage
def main():
    analytics = RealTimeAnalytics("/tmp/analytics")
    
    # Track page views
    analytics.increment("pageviews:home")
    analytics.increment("pageviews:products")
    analytics.increment("pageviews:checkout")
    
    # Record response times
    analytics.record_metric("response_time", 45.2, {"endpoint": "/api/users"})
    analytics.record_metric("response_time", 120.5, {"endpoint": "/api/products"})
    analytics.record_metric("response_time", 35.8, {"endpoint": "/api/users"})
    
    # Record memory usage
    analytics.record_metric("memory_usage", 2.5, {"unit": "GB"})
    
    # Get stats
    print(f"Home page views: {analytics.get_counter('pageviews:home')}")
    print(f"Avg response time (last 60min): {analytics.get_average('response_time', 60):.2f}ms")
    
    analytics.close()


if __name__ == "__main__":
    main()
```

### Connection Pool (C++)

```cpp
#include "fastcollection.h"
#include <iostream>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <chrono>
#include <functional>

using namespace fastcollection;

/**
 * Generic connection pool using FastCollection for persistence
 */
template<typename Connection>
class ConnectionPool {
public:
    using ConnectionFactory = std::function<std::unique_ptr<Connection>()>;
    using ConnectionValidator = std::function<bool(Connection*)>;

private:
    std::queue<std::unique_ptr<Connection>> pool;
    std::mutex mtx;
    std::condition_variable cv;
    
    size_t maxSize;
    size_t currentSize;
    ConnectionFactory factory;
    ConnectionValidator validator;
    
    // Track pool stats
    FastMap statsStore;
    
public:
    ConnectionPool(const std::string& statsPath,
                   size_t maxPoolSize,
                   ConnectionFactory factory,
                   ConnectionValidator validator = nullptr)
        : maxSize(maxPoolSize)
        , currentSize(0)
        , factory(std::move(factory))
        , validator(validator)
        , statsStore(statsPath, 16 * 1024 * 1024, true) {
        
        initStats();
    }
    
    void initStats() {
        std::string zero = "0";
        statsStore.put(
            reinterpret_cast<const uint8_t*>("acquired"), 8,
            reinterpret_cast<const uint8_t*>(zero.data()), zero.size()
        );
        statsStore.put(
            reinterpret_cast<const uint8_t*>("released"), 8,
            reinterpret_cast<const uint8_t*>(zero.data()), zero.size()
        );
    }
    
    void incrementStat(const std::string& name) {
        std::vector<uint8_t> result;
        if (statsStore.get(reinterpret_cast<const uint8_t*>(name.data()), name.size(), result)) {
            int64_t value = std::stoll(std::string(result.begin(), result.end())) + 1;
            std::string valueStr = std::to_string(value);
            statsStore.put(
                reinterpret_cast<const uint8_t*>(name.data()), name.size(),
                reinterpret_cast<const uint8_t*>(valueStr.data()), valueStr.size()
            );
        }
    }
    
    std::unique_ptr<Connection> acquire(std::chrono::milliseconds timeout = std::chrono::milliseconds(5000)) {
        std::unique_lock<std::mutex> lock(mtx);
        
        // Wait for available connection or ability to create new one
        auto deadline = std::chrono::steady_clock::now() + timeout;
        
        while (pool.empty() && currentSize >= maxSize) {
            if (cv.wait_until(lock, deadline) == std::cv_status::timeout) {
                throw std::runtime_error("Connection pool timeout");
            }
        }
        
        std::unique_ptr<Connection> conn;
        
        if (!pool.empty()) {
            conn = std::move(pool.front());
            pool.pop();
            
            // Validate connection
            if (validator && !validator(conn.get())) {
                currentSize--;
                conn = factory();
                currentSize++;
            }
        } else {
            conn = factory();
            currentSize++;
        }
        
        incrementStat("acquired");
        return conn;
    }
    
    void release(std::unique_ptr<Connection> conn) {
        std::lock_guard<std::mutex> lock(mtx);
        
        if (pool.size() < maxSize) {
            pool.push(std::move(conn));
        } else {
            currentSize--;
        }
        
        incrementStat("released");
        cv.notify_one();
    }
    
    size_t available() const {
        std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(mtx));
        return pool.size();
    }
    
    size_t size() const {
        return currentSize;
    }
};

// Example usage with a mock database connection
struct DbConnection {
    int id;
    bool connected;
    
    DbConnection() : id(rand()), connected(true) {
        std::cout << "Created connection " << id << std::endl;
    }
    
    ~DbConnection() {
        std::cout << "Destroyed connection " << id << std::endl;
    }
    
    void query(const std::string& sql) {
        std::cout << "Connection " << id << " executing: " << sql << std::endl;
    }
};

int main() {
    // Create pool with max 5 connections
    ConnectionPool<DbConnection> pool(
        "/tmp/connpool_stats.fc",
        5,
        []() { return std::make_unique<DbConnection>(); },
        [](DbConnection* conn) { return conn->connected; }
    );
    
    // Acquire and use connections
    {
        auto conn1 = pool.acquire();
        conn1->query("SELECT * FROM users");
        
        auto conn2 = pool.acquire();
        conn2->query("SELECT * FROM products");
        
        pool.release(std::move(conn1));
        pool.release(std::move(conn2));
    }
    
    std::cout << "Available connections: " << pool.available() << std::endl;
    
    return 0;
}
```

### Distributed Configuration Store (Java)

```java
import com.kuber.fastcollection.*;
import java.io.Serializable;
import java.util.*;
import java.util.concurrent.*;
import java.util.function.Consumer;

/**
 * Distributed configuration store with change notifications
 */
public class ConfigStore {
    
    public static class ConfigEntry implements Serializable {
        private static final long serialVersionUID = 1L;
        public String key;
        public String value;
        public String type;  // string, int, boolean, json
        public long version;
        public long updatedAt;
        public String updatedBy;
        
        public ConfigEntry(String key, String value, String type) {
            this.key = key;
            this.value = value;
            this.type = type;
            this.version = 1;
            this.updatedAt = System.currentTimeMillis();
        }
    }
    
    private final FastCollectionMap<String, ConfigEntry> configs;
    private final Map<String, List<Consumer<ConfigEntry>>> watchers = new ConcurrentHashMap<>();
    
    public ConfigStore(String path) {
        this.configs = new FastCollectionMap<>(path, 32 * 1024 * 1024, true);
    }
    
    public void set(String key, String value) {
        set(key, value, "string");
    }
    
    public void set(String key, String value, String type) {
        ConfigEntry existing = configs.get(key);
        ConfigEntry entry = new ConfigEntry(key, value, type);
        
        if (existing != null) {
            entry.version = existing.version + 1;
        }
        
        configs.put(key, entry);
        notifyWatchers(entry);
    }
    
    public String get(String key) {
        ConfigEntry entry = configs.get(key);
        return entry != null ? entry.value : null;
    }
    
    public String get(String key, String defaultValue) {
        String value = get(key);
        return value != null ? value : defaultValue;
    }
    
    public int getInt(String key, int defaultValue) {
        String value = get(key);
        return value != null ? Integer.parseInt(value) : defaultValue;
    }
    
    public boolean getBoolean(String key, boolean defaultValue) {
        String value = get(key);
        return value != null ? Boolean.parseBoolean(value) : defaultValue;
    }
    
    public long getVersion(String key) {
        ConfigEntry entry = configs.get(key);
        return entry != null ? entry.version : 0;
    }
    
    public void watch(String key, Consumer<ConfigEntry> callback) {
        watchers.computeIfAbsent(key, k -> new CopyOnWriteArrayList<>()).add(callback);
    }
    
    public void watchPrefix(String prefix, Consumer<ConfigEntry> callback) {
        // In production, maintain an index for prefix queries
        watchers.computeIfAbsent("prefix:" + prefix, k -> new CopyOnWriteArrayList<>()).add(callback);
    }
    
    private void notifyWatchers(ConfigEntry entry) {
        // Exact key watchers
        List<Consumer<ConfigEntry>> keyWatchers = watchers.get(entry.key);
        if (keyWatchers != null) {
            keyWatchers.forEach(w -> w.accept(entry));
        }
        
        // Prefix watchers
        watchers.forEach((pattern, callbacks) -> {
            if (pattern.startsWith("prefix:")) {
                String prefix = pattern.substring(7);
                if (entry.key.startsWith(prefix)) {
                    callbacks.forEach(w -> w.accept(entry));
                }
            }
        });
    }
    
    public void close() {
        configs.close();
    }
    
    // Usage example
    public static void main(String[] args) {
        ConfigStore config = new ConfigStore("/tmp/config.fc");
        
        // Set up watchers
        config.watch("feature.new_ui", entry -> {
            System.out.println("Feature flag changed: " + entry.key + " = " + entry.value);
        });
        
        config.watchPrefix("app.", entry -> {
            System.out.println("App config changed: " + entry.key + " = " + entry.value);
        });
        
        // Set configuration values
        config.set("app.name", "MyApp");
        config.set("app.version", "1.0.0");
        config.set("app.debug", "true", "boolean");
        config.set("app.max_connections", "100", "int");
        config.set("feature.new_ui", "false", "boolean");
        
        // Read configuration
        System.out.println("\nConfiguration:");
        System.out.println("  app.name: " + config.get("app.name"));
        System.out.println("  app.debug: " + config.getBoolean("app.debug", false));
        System.out.println("  app.max_connections: " + config.getInt("app.max_connections", 50));
        
        // Update (triggers watchers)
        System.out.println("\nEnabling new UI feature...");
        config.set("feature.new_ui", "true", "boolean");
        
        config.close();
    }
}
```

---

## Troubleshooting

### Native Library Issues

```java
// Debug native loading
NativeLibraryLoader.printDiagnostics();
```

### Performance Issues

```java
// Check file stats
FileStats stats = FastCollection.getFileStats(path);
System.out.println("Used: " + stats.usedSize + " / " + stats.totalSize);
```

### Data Corruption

```java
// Validate file
if (!FastCollection.isValidCollectionFile(path)) {
    // Delete and recreate
    new File(path).delete();
}
```

---

**FastCollection v1.0.0** - Quick Start Guide
