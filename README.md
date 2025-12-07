# FastCollection v1.0.0

**Ultra High-Performance Memory-Mapped Collections with TTL Support**

[![Version](https://img.shields.io/badge/Version-1.0.0-green.svg)](#)
[![License](https://img.shields.io/badge/License-Proprietary-red.svg)](LICENSE)
[![Patent Pending](https://img.shields.io/badge/Patent-Pending-blue.svg)](#legal)
[![Platforms](https://img.shields.io/badge/Platforms-Linux%20%7C%20macOS%20%7C%20Windows-blue.svg)](#supported-platforms)

Copyright © 2025-2030, Ashutosh Sinha (ajsinha@gmail.com)  
**Patent Pending** - All Rights Reserved

---

## Overview

FastCollection is an enterprise-grade library providing ultra high-performance collections backed by memory-mapped files. Version 1.0.0 introduces full cross-platform support with self-contained native libraries.

| Feature | Standard Collections | FastCollection |
|---------|---------------------|----------------|
| **Persistence** | ❌ Data lost on restart | ✅ Automatic persistence |
| **IPC** | ❌ Single process only | ✅ Multi-process sharing |
| **TTL Support** | ❌ Manual expiration | ✅ Built-in TTL per element |
| **Performance** | ~100ns | **~100-500ns** |
| **Memory Efficiency** | JVM heap | Memory-mapped (OS managed) |
| **Cross-Platform** | N/A | ✅ Linux, macOS, Windows |

## What's New in v1.0.0

- **Cross-Platform Support**: Single JAR works on Linux, macOS, and Windows
- **Static Linking**: Self-contained native libraries with no external dependencies at runtime
- **Multi-Architecture**: Support for x64, x86, arm64, and arm architectures
- **Enhanced Native Loader**: Automatic platform detection and library extraction
- **Improved Build System**: Platform-specific Maven profiles with optimized compiler flags

## Supported Platforms

| Platform | Architectures | Library Format |
|----------|---------------|----------------|
| **Linux** | x64, x86, arm64, arm | `.so` |
| **macOS** | x64, arm64 (Apple Silicon) | `.dylib` |
| **Windows** | x64, x86, arm64 | `.dll` |

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

### 🌐 Cross-Platform Native Libraries
- Single JAR contains native libraries for all platforms
- Automatic platform detection and library loading
- Statically linked - no runtime dependencies
- Transparent to end users

## Quick Start

### Maven Dependency

```xml
<dependency>
    <groupId>com.kuber</groupId>
    <artifactId>fastcollection</artifactId>
    <version>1.0.0</version>
</dependency>
```

### Gradle Dependency

```groovy
implementation 'com.kuber:fastcollection:1.0.0'
```

### Java Usage

```java
import com.kuber.fastcollection.*;

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

// Create a map
try (FastCollectionMap<String, String> map = 
        new FastCollectionMap<>("/tmp/mymap.fc")) {
    
    map.put("key", "value", 300);  // 5-minute TTL
    String value = map.get("key");
}

// Create a queue
try (FastCollectionQueue<Task> queue = 
        new FastCollectionQueue<>("/tmp/myqueue.fc")) {
    
    queue.offer(new Task(), 60);  // 1-minute TTL
    Task task = queue.poll();
}
```

### Debug Native Loading

```java
// Print diagnostic information about native library loading
NativeLibraryLoader.printDiagnostics();

// Check if loaded
if (NativeLibraryLoader.isLoaded()) {
    System.out.println("Platform: " + NativeLibraryLoader.getPlatform());
}
```

### C++ Usage

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

### Python Usage

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

## Building from Source

### Prerequisites

**All Platforms:**
- Java JDK 17+ with `JAVA_HOME` set
- Maven 3.8+
- CMake 3.18+
- C++20 compatible compiler

**Linux (Ubuntu/Debian):**
```bash
sudo apt-get install -y build-essential cmake libboost-all-dev openjdk-17-jdk
export JAVA_HOME=/usr/lib/jvm/java-17-openjdk-amd64
```

**macOS:**
```bash
brew install cmake boost openjdk@17
export JAVA_HOME=$(/usr/libexec/java_home -v 17)
```

**Windows:**
- Visual Studio 2022 with C++ workload
- Boost (via vcpkg): `vcpkg install boost:x64-windows-static`

### Single Platform Build

```bash
# Build for current platform (auto-detected)
mvn clean package

# The JAR will be in target/fastcollection-1.0.0.jar
# Native library in target/native/<platform>/<arch>/

# Release build (includes source and javadoc JARs)
mvn clean install -Prelease
```

### Multi-Platform Build

To create a single JAR containing native libraries for all platforms:

1. **Build on each platform:**

```bash
# On Linux x64
mvn clean package
cp target/native/linux/x64/*.so src/main/resources/native/linux/x64/

# On macOS arm64
mvn clean package
cp target/native/macos/arm64/*.dylib src/main/resources/native/macos/arm64/

# On Windows x64
mvn clean package
copy target\native\windows\x64\*.dll src\main\resources\native\windows\x64\
```

2. **Create multi-platform JAR:**

```bash
mvn clean package -Pmulti-platform -Pskip-native
```

### Build Profiles

| Profile | Description |
|---------|-------------|
| `linux-x64` | Linux 64-bit (auto-activated) |
| `linux-arm64` | Linux ARM64 |
| `macos-x64` | macOS Intel |
| `macos-arm64` | macOS Apple Silicon |
| `windows-x64` | Windows 64-bit |
| `windows-x86` | Windows 32-bit |
| `debug` | Debug build with symbols |
| `release` | Generate source and javadoc JARs |
| `multi-platform` | Package all native libs |
| `skip-native` | Skip native compilation |

### Verify Platform Detection

```bash
mvn help:active-profiles
```

## Library Output Structure

After building:

```
target/
├── fastcollection-1.0.0.jar          # Java classes + native libs
└── native/
    └── <platform>/
        └── <arch>/
            └── fastcollection-<platform>-<arch>.<ext>

# Example:
target/native/linux/x64/fastcollection-linux-x64.so
target/native/macos/arm64/fastcollection-macos-arm64.dylib
target/native/windows/x64/fastcollection-windows-x64.dll
```

With release profile (`mvn clean install -Prelease`):

```
target/
├── fastcollection-1.0.0.jar          # Java classes + native libs
├── fastcollection-1.0.0-sources.jar  # Source code (release profile only)
├── fastcollection-1.0.0-javadoc.jar  # Documentation (release profile only)
└── native/
    └── ...
```

## Native Library Loading

The `NativeLibraryLoader` automatically handles library loading:

1. **Check java.library.path** for platform-specific library
2. **Extract from JAR** to temp directory (cached)
3. **System.loadLibrary** as fallback

### Custom Library Path

```java
// Load from custom path (must call before any FastCollection class)
NativeLibraryLoader.loadFrom("/opt/myapp/lib/fastcollection-linux-x64.so");
```

### Troubleshooting

```java
// Print detailed diagnostics
NativeLibraryLoader.printDiagnostics();

// Output includes:
// - Detected platform and architecture
// - Expected library name
// - Search paths attempted
// - JAR resource locations checked
```

## TTL (Time-To-Live) Feature

### TTL Values

| Value | Meaning |
|-------|---------|
| `-1` (default) | Element never expires |
| `0` | Element expires immediately |
| `> 0` | Element expires after N seconds |

### TTL Operations

```java
// Add with TTL
list.add(element, 300);           // Expires in 5 minutes

// Check remaining TTL
long remaining = list.getTTL(0);  // Returns: seconds remaining, -1 if infinite

// Update TTL
list.setTTL(0, 600);              // Extend to 10 minutes

// Remove expired elements
int removed = list.removeExpired();
```

## Performance Characteristics

| Collection | Operation | Average Time | Complexity |
|------------|-----------|--------------|------------|
| **FastList** | add (tail) | < 500ns | O(1) |
| | get (sequential) | < 100ns | O(1) |
| **FastSet** | add/contains | < 300ns | O(1) avg |
| **FastMap** | put/get | < 400ns | O(1) avg |
| **FastQueue** | offer/poll | < 200ns | O(1) |
| **FastStack** | push/pop | < 150ns | O(1) |

## API Reference

See [docs/API.md](docs/API.md) for complete API documentation.

## Quick Start & Examples

See [docs/QUICKSTART.md](docs/QUICKSTART.md) for comprehensive examples including:

- **Java**: 10+ complete examples (caching, session management, rate limiting, event logging, etc.)
- **Python**: 8+ complete examples with context managers and TTL patterns
- **C++**: 6+ complete examples with RAII and serialization patterns

## Architecture

See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for technical details.

## Legal

**Copyright © 2025-2030, Ashutosh Sinha (ajsinha@gmail.com)**

This software and its documentation are proprietary and confidential. Unauthorized copying, distribution, modification, or use is strictly prohibited without explicit written permission from the copyright holder.

**Patent Pending**: The architectural patterns and implementations described in this library may be subject to patent applications. Commercial use requires licensing.

## Support

For licensing inquiries or technical support, contact: ajsinha@gmail.com

---

**FastCollection v1.0.0** - Ultra High-Performance Collections
