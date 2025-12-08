# FastCollection v1.0.0 Comprehensive Build Guide

## Table of Contents

1. [Overview](#overview)
2. [Supported Platforms](#supported-platforms)
3. [Prerequisites](#prerequisites)
   - [Linux (Ubuntu/Debian)](#linux-ubuntudebian)
   - [Linux (RHEL/CentOS/Fedora)](#linux-rhelcentosfedora)
   - [Linux (Arch Linux)](#linux-arch-linux)
   - [macOS](#macos)
   - [Windows](#windows)
4. [Building from Source](#building-from-source)
   - [Quick Build](#quick-build)
   - [Build Commands Reference](#build-commands-reference)
   - [Build Profiles](#build-profiles)
5. [Verifying the Build](#verifying-the-build)
   - [Check Build Artifacts](#check-build-artifacts)
   - [Verify Native Library Bundling](#verify-native-library-bundling)
   - [Run Diagnostics](#run-diagnostics)
   - [Run Unit Tests](#run-unit-tests)
6. [Running Examples](#running-examples)
   - [Java Examples](#java-examples)
   - [Python Examples](#python-examples)
   - [C++ Examples](#cpp-examples)
7. [Using FastCollection in Your Project](#using-fastcollection-in-your-project)
   - [Maven Dependency](#maven-dependency)
   - [Gradle Dependency](#gradle-dependency)
   - [Manual JAR Usage](#manual-jar-usage)
8. [Multi-Platform JAR](#multi-platform-jar)
9. [CI/CD Integration](#cicd-integration)
10. [Troubleshooting](#troubleshooting)
11. [Performance Optimization](#performance-optimization)
12. [Support](#support)

---

## Overview

FastCollection uses a hybrid build system:
- **Maven** orchestrates the entire build process
- **CMake** compiles the native C++ code
- **JNI** bridges Java and native code

### Build Process Flow

```
┌─────────────────────────────────────────────────────────────────┐
│                    mvn clean package                             │
├─────────────────────────────────────────────────────────────────┤
│ 1. Clean previous build artifacts                                │
│ 2. Compile Java source files                                     │
│ 3. Run CMake to configure native build                           │
│ 4. Compile C++ code → native library (.so/.dylib/.dll)           │
│ 5. Copy native library into target/classes/native/               │
│ 6. Package everything into JAR                                   │
│ 7. Run tests (unless -DskipTests)                                │
└─────────────────────────────────────────────────────────────────┘
```

### What You Get

After a successful build:
```
target/
├── fastcollection-1.0.0.jar          # Self-contained JAR with native lib
├── classes/
│   └── native/linux/x64/             # Native lib bundled for packaging
│       └── fastcollection-linux-x64.so
└── native/linux/x64/                 # Compiled native library
    └── fastcollection-linux-x64.so
```

The JAR is **self-contained** - it includes the native library which is automatically extracted and loaded at runtime.

---

## Supported Platforms

| Platform | Architecture | Compiler | Native Library |
|----------|--------------|----------|----------------|
| Linux | x64 (amd64) | GCC 10+ | `fastcollection-linux-x64.so` |
| Linux | x86 (i386) | GCC 10+ | `fastcollection-linux-x86.so` |
| Linux | arm64 (aarch64) | GCC 10+ | `fastcollection-linux-arm64.so` |
| Linux | arm (armhf) | GCC 10+ | `fastcollection-linux-arm.so` |
| macOS | x64 (Intel) | Clang 13+ | `fastcollection-macos-x64.dylib` |
| macOS | arm64 (M1/M2/M3) | Clang 13+ | `fastcollection-macos-arm64.dylib` |
| Windows | x64 | MSVC 2022 | `fastcollection-windows-x64.dll` |
| Windows | x86 | MSVC 2022 | `fastcollection-windows-x86.dll` |
| Windows | arm64 | MSVC 2022 | `fastcollection-windows-arm64.dll` |

---

## Prerequisites

### Required Software (All Platforms)

| Software | Minimum Version | Purpose |
|----------|-----------------|---------|
| Java JDK | 17+ | Java compilation, JNI headers |
| Maven | 3.8+ | Build orchestration |
| CMake | 3.18+ | Native build configuration |
| Boost | 1.74+ | Interprocess, thread, system libs |
| C++ Compiler | C++20 support | Native code compilation |

### Linux (Ubuntu/Debian)

```bash
# Update package list
sudo apt-get update

# Install all build dependencies
sudo apt-get install -y \
    build-essential \
    cmake \
    libboost-all-dev \
    openjdk-17-jdk \
    maven

# Set JAVA_HOME environment variable
export JAVA_HOME=/usr/lib/jvm/java-17-openjdk-amd64

# Make it permanent
echo 'export JAVA_HOME=/usr/lib/jvm/java-17-openjdk-amd64' >> ~/.bashrc
source ~/.bashrc

# Verify all installations
echo "=== Checking Java ==="
java -version
echo ""
echo "=== Checking Maven ==="
mvn -version
echo ""
echo "=== Checking CMake ==="
cmake --version
echo ""
echo "=== Checking GCC ==="
g++ --version
echo ""
echo "=== Checking Boost ==="
dpkg -l | grep libboost-dev
```

**Expected Output:**
```
=== Checking Java ===
openjdk version "17.0.x" ...

=== Checking Maven ===
Apache Maven 3.8.x ...

=== Checking CMake ===
cmake version 3.22.x ...

=== Checking GCC ===
g++ (Ubuntu 11.x.x) ...

=== Checking Boost ===
ii  libboost-dev  1.74.0 ...
```

### Linux (RHEL/CentOS/Fedora)

```bash
# Fedora / RHEL 8+
sudo dnf install -y \
    gcc-c++ \
    cmake \
    boost-devel \
    java-17-openjdk-devel \
    maven

# Set JAVA_HOME
export JAVA_HOME=/usr/lib/jvm/java-17-openjdk
echo 'export JAVA_HOME=/usr/lib/jvm/java-17-openjdk' >> ~/.bashrc
source ~/.bashrc

# Verify
java -version
mvn -version
cmake --version
```

### Linux (Arch Linux)

```bash
# Install all dependencies
sudo pacman -S base-devel cmake boost jdk17-openjdk maven

# Set JAVA_HOME
export JAVA_HOME=/usr/lib/jvm/java-17-openjdk
echo 'export JAVA_HOME=/usr/lib/jvm/java-17-openjdk' >> ~/.bashrc
source ~/.bashrc
```

### macOS

```bash
# Step 1: Install Xcode Command Line Tools
xcode-select --install

# Step 2: Install Homebrew (if not already installed)
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Step 3: Install dependencies
brew install cmake boost maven openjdk@17

# Step 4: Set JAVA_HOME
export JAVA_HOME=$(/usr/libexec/java_home -v 17)

# Make it permanent (for zsh - default on modern macOS)
echo 'export JAVA_HOME=$(/usr/libexec/java_home -v 17)' >> ~/.zshrc
source ~/.zshrc

# Or for bash
echo 'export JAVA_HOME=$(/usr/libexec/java_home -v 17)' >> ~/.bash_profile
source ~/.bash_profile

# Verify installations
echo "=== Checking Java ==="
java -version
echo ""
echo "=== Checking Maven ==="
mvn -version
echo ""
echo "=== Checking CMake ==="
cmake --version
echo ""
echo "=== Checking Clang ==="
clang++ --version
echo ""
echo "=== Checking Boost ==="
brew info boost
```

### Windows

#### Step 1: Install Visual Studio 2022

1. Download Visual Studio 2022 Community (free) from:
   https://visualstudio.microsoft.com/downloads/

2. Run the installer

3. Select **"Desktop development with C++"** workload

4. In the right panel, ensure these are checked:
   - ✅ MSVC v143 - VS 2022 C++ x64/x86 build tools
   - ✅ C++ CMake tools for Windows
   - ✅ Windows 10/11 SDK

5. Click Install (requires ~8GB disk space)

#### Step 2: Install CMake (if not included with VS)

1. Download from https://cmake.org/download/
2. Run installer
3. Select "Add CMake to system PATH"

#### Step 3: Install Boost

**Option A: Using vcpkg (Recommended)**

```powershell
# Open PowerShell as Administrator

# Clone vcpkg
git clone https://github.com/Microsoft/vcpkg.git C:\vcpkg
cd C:\vcpkg

# Bootstrap vcpkg
.\bootstrap-vcpkg.bat

# Install Boost (takes 10-20 minutes)
.\vcpkg install boost:x64-windows-static

# Integrate with Visual Studio
.\vcpkg integrate install

# Set BOOST_ROOT environment variable
[Environment]::SetEnvironmentVariable("BOOST_ROOT", "C:\vcpkg\installed\x64-windows-static", "User")
```

**Option B: Pre-built Boost Binaries**

1. Download from https://www.boost.org/users/download/
2. Extract to `C:\boost_1_83_0`
3. Open Command Prompt in that directory:
   ```powershell
   cd C:\boost_1_83_0
   .\bootstrap.bat
   .\b2 link=static runtime-link=static variant=release
   ```
4. Set environment variable:
   ```powershell
   [Environment]::SetEnvironmentVariable("BOOST_ROOT", "C:\boost_1_83_0", "User")
   ```

#### Step 4: Install JDK 17

1. Download Eclipse Temurin JDK 17 from https://adoptium.net/
2. Run installer with default options
3. The installer should set JAVA_HOME automatically
4. Verify: Open new PowerShell and run `java -version`

If JAVA_HOME is not set:
```powershell
[Environment]::SetEnvironmentVariable("JAVA_HOME", "C:\Program Files\Eclipse Adoptium\jdk-17.0.x-hotspot", "User")
[Environment]::SetEnvironmentVariable("Path", $env:Path + ";%JAVA_HOME%\bin", "User")
```

#### Step 5: Install Maven

1. Download from https://maven.apache.org/download.cgi (Binary zip archive)
2. Extract to `C:\maven`
3. Add to PATH:
   ```powershell
   [Environment]::SetEnvironmentVariable("Path", $env:Path + ";C:\maven\bin", "User")
   ```

#### Step 6: Verify Windows Installation

**Important**: Open **"Developer Command Prompt for VS 2022"** (not regular PowerShell):

```powershell
echo === Java ===
java -version

echo === Maven ===
mvn -version

echo === CMake ===
cmake --version

echo === MSVC ===
cl

echo === BOOST_ROOT ===
echo %BOOST_ROOT%
```

---

## Building from Source

### Quick Build

```bash
# Clone the repository
git clone https://github.com/abhikarta/fastcollection.git
cd fastcollection

# Build (auto-detects your platform)
mvn clean package

# Check the output
ls -la target/fastcollection-1.0.0.jar
```

**On Windows** (use Developer Command Prompt for VS 2022):
```powershell
git clone https://github.com/abhikarta/fastcollection.git
cd fastcollection
mvn clean package
dir target\fastcollection-1.0.0.jar
```

### Build Commands Reference

| Command | Description |
|---------|-------------|
| `mvn clean package` | Standard release build |
| `mvn clean package -DskipTests` | Build without running tests |
| `mvn clean package -Pdebug` | Debug build with symbols |
| `mvn clean install` | Build and install to local Maven repository |
| `mvn clean install -Prelease` | Build with source and javadoc JARs |
| `mvn clean package -Pskip-native` | Java-only build (skip C++ compilation) |
| `mvn clean package -X` | Verbose output for debugging |
| `mvn clean package -T 4` | Parallel build using 4 threads |

### Build Profiles

Maven automatically activates platform-specific profiles based on your OS:

| Profile | Auto-Activated When | Description |
|---------|---------------------|-------------|
| `linux-x64` | Linux + amd64 | Linux 64-bit build |
| `linux-x86` | Linux + i386 | Linux 32-bit build |
| `linux-arm64` | Linux + aarch64 | Linux ARM64 build |
| `macos-x64` | macOS + x86_64 | macOS Intel build |
| `macos-arm64` | macOS + aarch64 | macOS Apple Silicon build |
| `windows-x64` | Windows + amd64 | Windows 64-bit build |
| `windows-x86` | Windows + x86 | Windows 32-bit build |

**Manually-activated profiles:**

| Profile | Command | Description |
|---------|---------|-------------|
| `debug` | `-Pdebug` | Debug build with symbols and sanitizers |
| `release` | `-Prelease` | Generate source and javadoc JARs |
| `skip-native` | `-Pskip-native` | Skip native C++ compilation |
| `multi-platform` | `-Pmulti-platform` | Include all pre-built native libraries |
| `python` | `-Ppython` | Also build Python bindings |

### Check Active Profiles

```bash
mvn help:active-profiles
```

**Expected output:**
```
Active Profiles for Project com.kuber:fastcollection:jar:1.0.0:
  linux-x64 (source: pom)
```

---

## Verifying the Build

### Check Build Artifacts

```bash
# Check JAR exists
ls -la target/fastcollection-1.0.0.jar

# Check native library was compiled
ls -la target/native/

# Example output on Linux x64:
# target/native/linux/x64/fastcollection-linux-x64.so
```

### Verify Native Library Bundling

The native library must be bundled INSIDE the JAR for it to work:

```bash
# List JAR contents and look for native library
jar tf target/fastcollection-1.0.0.jar | grep native

# Expected output:
# native/linux/x64/fastcollection-linux-x64.so

# More detailed view
unzip -l target/fastcollection-1.0.0.jar | grep native

# If you don't see the native library, rebuild:
mvn clean package
```

**Windows:**
```powershell
jar tf target\fastcollection-1.0.0.jar | findstr native
```

### Run Diagnostics

Create a test file to verify native loading:

**TestNative.java:**
```java
import com.kuber.fastcollection.NativeLibraryLoader;
import com.kuber.fastcollection.FastCollectionList;

public class TestNative {
    public static void main(String[] args) {
        System.out.println("=== FastCollection Native Library Diagnostics ===\n");
        
        // Print diagnostic information
        NativeLibraryLoader.printDiagnostics();
        
        System.out.println("\n=== Attempting to Load Library ===");
        try {
            NativeLibraryLoader.load();
            System.out.println("✓ SUCCESS: Native library loaded!\n");
            
            // Quick functionality test
            System.out.println("=== Quick Functionality Test ===");
            FastCollectionList<String> list = new FastCollectionList<>("/tmp/test.fc", String.class);
            list.add("Hello");
            list.add("World");
            System.out.println("List size: " + list.size());
            System.out.println("Element 0: " + list.get(0));
            System.out.println("Element 1: " + list.get(1));
            list.close();
            System.out.println("✓ All tests passed!");
            
            // Cleanup
            new java.io.File("/tmp/test.fc").delete();
            
        } catch (UnsatisfiedLinkError e) {
            System.out.println("✗ FAILED: " + e.getMessage());
            System.exit(1);
        } catch (Exception e) {
            System.out.println("✗ ERROR: " + e.getMessage());
            e.printStackTrace();
            System.exit(1);
        }
    }
}
```

**Compile and run:**
```bash
# Compile
javac -cp target/fastcollection-1.0.0.jar TestNative.java

# Run
java -cp target/fastcollection-1.0.0.jar:. TestNative

# Windows uses semicolon:
java -cp "target\fastcollection-1.0.0.jar;." TestNative
```

**Expected output:**
```
=== FastCollection Native Library Diagnostics ===

Platform Information:
  OS: Linux
  Architecture: amd64
  Detected Platform: linux-x64

Library Search:
  Expected library name: fastcollection-linux-x64.so
  JAR resource paths to check:
    /native/linux/x64/fastcollection-linux-x64.so
    /native/linux/x64/libfastcollection-linux-x64.so

=== Attempting to Load Library ===
✓ SUCCESS: Native library loaded!

=== Quick Functionality Test ===
List size: 2
Element 0: Hello
Element 1: World
✓ All tests passed!
```

### Run Unit Tests

```bash
# Run all tests
mvn test

# Run a specific test class
mvn test -Dtest=FastCollectionListTest

# Run tests with debug output
mvn test -Dfastcollection.debug=true

# Run tests with custom library path (if needed)
mvn test -Djava.library.path=target/native/linux/x64
```

---

## Running Examples

### Example Files Overview

```
examples/
├── java/
│   ├── BasicExample.java           # Simple list operations
│   ├── CacheExample.java           # Key-value cache with TTL
│   ├── SessionManagerExample.java  # HTTP session storage
│   ├── RateLimiterExample.java     # Sliding window rate limiting
│   ├── TaskQueueExample.java       # Task queue with retry logic
│   ├── EventLogExample.java        # Time-series event logging
│   ├── LeaderboardExample.java     # Game leaderboard rankings
│   ├── ShoppingCartExample.java    # E-commerce shopping cart
│   └── DistributedLockExample.java # Distributed locking
├── python/
│   ├── basic_example.py
│   ├── cache_example.py
│   ├── session_manager_example.py
│   ├── rate_limiter_example.py
│   ├── task_queue_example.py
│   └── event_log_example.py
└── cpp/
    ├── basic_example.cpp
    ├── cache_example.cpp
    └── task_queue_example.cpp
```

### Java Examples

#### Step 1: Build the Project First

```bash
cd /path/to/fastcollection
mvn clean package -DskipTests
```

#### Step 2: Navigate to Examples Directory

```bash
cd examples/java
```

#### Step 3: Compile Examples

```bash
# Compile all Java examples
javac -cp ../../target/fastcollection-1.0.0.jar *.java
```

**Windows:**
```powershell
javac -cp "..\..\target\fastcollection-1.0.0.jar" *.java
```

#### Step 4: Run Examples

**Linux/macOS:**
```bash
# BasicExample - Simple list operations
java -cp ../../target/fastcollection-1.0.0.jar:. BasicExample

# CacheExample - TTL-based cache
java -cp ../../target/fastcollection-1.0.0.jar:. CacheExample

# SessionManagerExample - HTTP sessions
java -cp ../../target/fastcollection-1.0.0.jar:. SessionManagerExample

# RateLimiterExample - Rate limiting
java -cp ../../target/fastcollection-1.0.0.jar:. RateLimiterExample

# TaskQueueExample - Task queue
java -cp ../../target/fastcollection-1.0.0.jar:. TaskQueueExample

# EventLogExample - Event logging
java -cp ../../target/fastcollection-1.0.0.jar:. EventLogExample

# LeaderboardExample - Game leaderboard
java -cp ../../target/fastcollection-1.0.0.jar:. LeaderboardExample

# ShoppingCartExample - Shopping cart
java -cp ../../target/fastcollection-1.0.0.jar:. ShoppingCartExample

# DistributedLockExample - Distributed locks
java -cp ../../target/fastcollection-1.0.0.jar:. DistributedLockExample
```

**Windows (use semicolon for classpath):**
```powershell
java -cp "..\..\target\fastcollection-1.0.0.jar;." BasicExample
java -cp "..\..\target\fastcollection-1.0.0.jar;." CacheExample
java -cp "..\..\target\fastcollection-1.0.0.jar;." SessionManagerExample
# ... etc
```

#### Alternative: Run from Project Root

```bash
cd /path/to/fastcollection

# Build
mvn clean package -DskipTests

# Create examples output directory
mkdir -p target/examples

# Compile examples to target directory
javac -cp target/fastcollection-1.0.0.jar \
      -d target/examples \
      examples/java/*.java

# Run examples
java -cp target/fastcollection-1.0.0.jar:target/examples BasicExample
java -cp target/fastcollection-1.0.0.jar:target/examples CacheExample
# ... etc
```

#### Alternative: Using Maven exec:java

```bash
cd /path/to/fastcollection

# Add examples to classpath and run
mvn exec:java \
    -Dexec.mainClass="BasicExample" \
    -Dexec.sourceRoot="examples/java" \
    -Dexec.classpathScope="compile"
```

### Example Output: BasicExample

```
=== FastCollection Basic Example ===

Creating FastCollectionList...
Adding elements...
List size: 5
Element at index 0: First Element
Element at index 2: Third Element

Iterating over list:
  [0] First Element
  [1] Second Element
  [2] Third Element
  [3] Fourth Element
  [4] Fifth Element

Contains 'Third Element': true
Index of 'Fourth Element': 3

Removing element at index 2...
List size after removal: 4

List closed successfully.
Data file location: /tmp/basic_example.fc
```

### Example Output: CacheExample

```
=== FastCollection Cache Example ===

Creating cache with 60-second TTL...
Adding cache entries...

Cache contents:
  user:1001 -> {"name":"Alice","email":"alice@example.com"}
  user:1002 -> {"name":"Bob","email":"bob@example.com"}
  session:abc123 -> {"userId":1001,"loginTime":"2024-01-15T10:30:00"}

Getting entry: user:1001 -> {"name":"Alice","email":"alice@example.com"}

Waiting for TTL expiration demo...
Entry after TTL: null (expired)

Cache statistics:
  Total entries: 3
  Expired entries removed: 1

Cache closed.
```

### Python Examples

#### Prerequisites

The Python module requires:
- Python 3.8+
- pybind11 >= 2.6
- C++ compiler (same as for Java build)
- Boost development headers

```bash
# Install Python build dependencies
pip install pybind11 setuptools wheel

# Navigate to project root
cd /path/to/fastcollection

# Option 1: Install in development mode (recommended for testing)
pip install -e .

# Option 2: Build and install
pip install .

# Option 3: Build wheel for distribution
pip wheel . -w dist/

# Option 4: Using Maven (builds Python module alongside Java)
mvn clean package -Ppython
```

#### Verify Python Installation

```python
# Test import
python -c "from fastcollection import FastList, FastMap, TTL_INFINITE; print('Success!')"

# Check version
python -c "import fastcollection; print(fastcollection.__version__)"
```

#### Run Python Examples

```bash
cd examples/python

# Basic operations
python basic_example.py

# Cache with TTL
python cache_example.py

# Session management
python session_manager_example.py

# Rate limiting
python rate_limiter_example.py

# Task queue
python task_queue_example.py

# Event logging
python event_log_example.py
```

#### Python Module Structure

After installation, the module structure is:
```
fastcollection/
├── __init__.py          # Python package init (imports from _native)
└── _native.cpython-*.so # Compiled native extension
```

#### Python Troubleshooting

**Import Error: "Native module not found"**
```bash
# Rebuild the module
pip uninstall fastcollection
pip install -e .
```

**Compilation Error: "pybind11 not found"**
```bash
pip install pybind11
```

**Compilation Error: "boost headers not found"**
```bash
# Linux
sudo apt-get install libboost-all-dev

# macOS
brew install boost
```

### C++ Examples

#### Build C++ Examples

```bash
cd examples/cpp
mkdir build && cd build

# Configure CMake
cmake .. \
    -DFASTCOLLECTION_INCLUDE_DIR=../../../src/main/cpp/include \
    -DFASTCOLLECTION_LIB_DIR=../../../target/native/linux/x64

# Build
make

# Run
./basic_example
./cache_example
./task_queue_example
```

#### Example CMakeLists.txt for C++ Examples

```cmake
cmake_minimum_required(VERSION 3.18)
project(fastcollection_examples)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find FastCollection
find_path(FASTCOLLECTION_INCLUDE_DIR fastcollection.h
    HINTS ${CMAKE_SOURCE_DIR}/../../../src/main/cpp/include)

find_library(FASTCOLLECTION_LIB fastcollection
    HINTS ${CMAKE_SOURCE_DIR}/../../../target/native/linux/x64)

# Build examples
add_executable(basic_example basic_example.cpp)
target_include_directories(basic_example PRIVATE ${FASTCOLLECTION_INCLUDE_DIR})
target_link_libraries(basic_example ${FASTCOLLECTION_LIB} pthread)

add_executable(cache_example cache_example.cpp)
target_include_directories(cache_example PRIVATE ${FASTCOLLECTION_INCLUDE_DIR})
target_link_libraries(cache_example ${FASTCOLLECTION_LIB} pthread)
```

---

## Using FastCollection in Your Project

### Maven Dependency

Add to your `pom.xml`:

```xml
<dependency>
    <groupId>com.kuber</groupId>
    <artifactId>fastcollection</artifactId>
    <version>1.0.0</version>
</dependency>
```

For local development (after running `mvn install`):
```bash
# Install to local Maven repository
cd /path/to/fastcollection
mvn clean install -DskipTests

# Now you can use it in other Maven projects
```

### Gradle Dependency

```groovy
implementation 'com.kuber:fastcollection:1.0.0'
```

### Manual JAR Usage

```bash
# Copy the JAR to your project
cp target/fastcollection-1.0.0.jar /path/to/your/project/lib/

# Compile your code
javac -cp lib/fastcollection-1.0.0.jar YourApp.java

# Run
java -cp lib/fastcollection-1.0.0.jar:. YourApp
```

### Minimal Example Application

**MyApp.java:**
```java
import com.kuber.fastcollection.*;

public class MyApp {
    public static void main(String[] args) {
        try (FastCollectionMap<String, String> cache = 
                new FastCollectionMap<>("/tmp/mycache.fc")) {
            
            // Store with 5-minute TTL
            cache.put("greeting", "Hello, World!", 300);
            
            // Retrieve
            System.out.println(cache.get("greeting"));
            
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
}
```

**Compile and run:**
```bash
javac -cp fastcollection-1.0.0.jar MyApp.java
java -cp fastcollection-1.0.0.jar:. MyApp
```

---

## Multi-Platform JAR

To create a single JAR that works on all platforms:

### Step 1: Build on Each Platform

**On Linux x64:**
```bash
mvn clean package -DskipTests
mkdir -p collected-natives/linux/x64
cp target/native/linux/x64/*.so collected-natives/linux/x64/
```

**On macOS arm64 (Apple Silicon):**
```bash
mvn clean package -DskipTests
mkdir -p collected-natives/macos/arm64
cp target/native/macos/arm64/*.dylib collected-natives/macos/arm64/
```

**On macOS x64 (Intel):**
```bash
mvn clean package -DskipTests
mkdir -p collected-natives/macos/x64
cp target/native/macos/x64/*.dylib collected-natives/macos/x64/
```

**On Windows x64:**
```powershell
mvn clean package -DskipTests
mkdir collected-natives\windows\x64
copy target\native\windows\x64\*.dll collected-natives\windows\x64\
```

### Step 2: Collect All Native Libraries

Copy all collected natives to `src/main/resources/native/`:

```
src/main/resources/native/
├── linux/
│   ├── x64/
│   │   └── fastcollection-linux-x64.so
│   └── arm64/
│       └── fastcollection-linux-arm64.so
├── macos/
│   ├── x64/
│   │   └── fastcollection-macos-x64.dylib
│   └── arm64/
│       └── fastcollection-macos-arm64.dylib
└── windows/
    └── x64/
        └── fastcollection-windows-x64.dll
```

### Step 3: Build Multi-Platform JAR

```bash
mvn clean package -Pmulti-platform -Pskip-native

# Verify all libraries are included
jar tf target/fastcollection-1.0.0.jar | grep native
```

---

## CI/CD Integration

### GitHub Actions Workflow

Create `.github/workflows/build.yml`:

```yaml
name: Build FastCollection

on:
  push:
    branches: [ main, develop ]
  pull_request:
    branches: [ main ]
  release:
    types: [ created ]

jobs:
  build-native:
    strategy:
      matrix:
        include:
          - os: ubuntu-latest
            platform: linux
            arch: x64
          - os: ubuntu-24.04-arm
            platform: linux
            arch: arm64
          - os: macos-latest
            platform: macos
            arch: arm64
          - os: macos-13
            platform: macos
            arch: x64
          - os: windows-latest
            platform: windows
            arch: x64

    runs-on: ${{ matrix.os }}

    steps:
    - uses: actions/checkout@v4

    - name: Set up JDK 17
      uses: actions/setup-java@v4
      with:
        java-version: '17'
        distribution: 'temurin'

    - name: Install Boost (Linux)
      if: runner.os == 'Linux'
      run: sudo apt-get update && sudo apt-get install -y libboost-all-dev

    - name: Install Boost (macOS)
      if: runner.os == 'macOS'
      run: brew install boost

    - name: Install Boost (Windows)
      if: runner.os == 'Windows'
      run: |
        vcpkg install boost:x64-windows-static
        echo "BOOST_ROOT=$env:VCPKG_INSTALLATION_ROOT\installed\x64-windows-static" >> $env:GITHUB_ENV

    - name: Build
      run: mvn clean package -DskipTests

    - name: Run Tests
      run: mvn test

    - name: Upload native library
      uses: actions/upload-artifact@v4
      with:
        name: native-${{ matrix.platform }}-${{ matrix.arch }}
        path: target/native/

  package-multiplatform:
    needs: build-native
    runs-on: ubuntu-latest

    steps:
    - uses: actions/checkout@v4

    - name: Set up JDK 17
      uses: actions/setup-java@v4
      with:
        java-version: '17'
        distribution: 'temurin'

    - name: Download all native libraries
      uses: actions/download-artifact@v4
      with:
        path: src/main/resources/native/
        pattern: native-*
        merge-multiple: true

    - name: Build multi-platform JAR
      run: mvn clean package -Pmulti-platform -Pskip-native -Prelease

    - name: Upload multi-platform JAR
      uses: actions/upload-artifact@v4
      with:
        name: fastcollection-multiplatform
        path: |
          target/fastcollection-1.0.0.jar
          target/fastcollection-1.0.0-sources.jar
          target/fastcollection-1.0.0-javadoc.jar
```

---

## Troubleshooting

### Issue: "UnsatisfiedLinkError: Failed to load FastCollection native library"

**Step 1: Run diagnostics**
```java
import com.kuber.fastcollection.NativeLibraryLoader;
NativeLibraryLoader.printDiagnostics();
```

**Step 2: Check if native library is in JAR**
```bash
jar tf target/fastcollection-1.0.0.jar | grep native
# Should output: native/linux/x64/fastcollection-linux-x64.so
```

**Step 3: If library is missing, rebuild**
```bash
mvn clean package
```

**Step 4: Check library dependencies (Linux)**
```bash
ldd target/native/linux/x64/fastcollection-linux-x64.so
# All dependencies should show paths, not "not found"
```

### Issue: CMake not found

```bash
# Linux
sudo apt-get install cmake

# macOS
brew install cmake

# Windows
# Install Visual Studio with C++ CMake tools
```

### Issue: Boost not found

**Linux:**
```bash
sudo apt-get install libboost-all-dev
# Verify
ls /usr/include/boost/
```

**macOS:**
```bash
brew install boost
export BOOST_ROOT=$(brew --prefix boost)
```

**Windows:**
```powershell
# Check vcpkg installation
vcpkg list | findstr boost

# Verify BOOST_ROOT
echo %BOOST_ROOT%
dir %BOOST_ROOT%\include\boost
```

### Issue: JNI headers not found

```bash
# Check JAVA_HOME
echo $JAVA_HOME
ls $JAVA_HOME/include/jni.h

# If file doesn't exist, JAVA_HOME points to JRE instead of JDK
# Fix by installing JDK:
sudo apt-get install openjdk-17-jdk
export JAVA_HOME=/usr/lib/jvm/java-17-openjdk-amd64
```

### Issue: Windows build fails

1. **Use Developer Command Prompt** (not regular PowerShell or CMD)
   - Start Menu → "Developer Command Prompt for VS 2022"

2. **Check Visual Studio installation:**
   ```powershell
   vswhere -property installationPath
   ```

3. **Specify CMake generator explicitly:**
   ```powershell
   mvn clean package -Dcmake.generator="Visual Studio 17 2022"
   ```

### Issue: Tests fail with library not found

```bash
# Set library path explicitly
export LD_LIBRARY_PATH=target/native/linux/x64:$LD_LIBRARY_PATH
mvn test

# Or pass to Maven
mvn test -Djava.library.path=target/native/linux/x64
```

### Enable Debug Logging

```bash
# Build with debug logging
mvn clean package -X 2>&1 | tee build.log

# Search for errors
grep -i error build.log
grep -i cmake build.log
grep -i boost build.log
```

---

## Performance Optimization

### Release Build Flags

| Platform | Optimization Flags |
|----------|-------------------|
| Linux/GCC | `-O3 -march=native -flto -ffast-math` |
| macOS/Clang | `-O3 -flto -ffast-math` |
| Windows/MSVC | `/O2 /GL /LTCG` |

### Portable Build

For distribution to various CPUs:

```bash
# Build without CPU-specific optimizations
mvn clean package -Dcmake.cxx.flags="-O3 -march=x86-64"
```

### Static Linking

The build produces statically-linked libraries:

| Component | Linux | macOS | Windows |
|-----------|-------|-------|---------|
| Boost | Static | Static | Static |
| C++ Runtime | Static | System | Static |
| C Runtime | Static | System | Static |

---

## Directory Structure

```
fastcollection/
├── pom.xml                          # Maven build configuration
├── src/
│   ├── main/
│   │   ├── java/com/kuber/fastcollection/
│   │   │   ├── FastCollectionList.java
│   │   │   ├── FastCollectionMap.java
│   │   │   ├── FastCollectionSet.java
│   │   │   ├── FastCollectionQueue.java
│   │   │   ├── FastCollectionStack.java
│   │   │   ├── FastCollectionException.java
│   │   │   └── NativeLibraryLoader.java
│   │   ├── cpp/
│   │   │   ├── CMakeLists.txt
│   │   │   ├── src/                 # C++ source files
│   │   │   ├── include/             # C++ headers
│   │   │   └── jni/                 # JNI bindings
│   │   └── resources/native/        # Pre-built natives (optional)
│   └── test/java/                   # Unit tests
├── examples/
│   ├── java/                        # Java examples
│   ├── python/                      # Python examples
│   └── cpp/                         # C++ examples
├── docs/
│   ├── BUILD.md                     # This file
│   ├── API.md                       # API documentation
│   ├── QUICKSTART.md                # Quick start guide
│   └── ARCHITECTURE.md              # Architecture details
└── target/                          # Build output
    ├── fastcollection-1.0.0.jar     # Main JAR (includes native)
    ├── classes/native/              # Bundled native lib
    ├── native/                      # Compiled native lib
    └── cmake-build/                 # CMake build files
```

---

## Quick Reference Card

### Build Commands

```bash
mvn clean package                    # Standard build
mvn clean package -DskipTests        # Skip tests
mvn clean package -Pdebug            # Debug build
mvn clean install -Prelease          # With sources/javadoc
mvn clean package -X                 # Verbose logging
```

### Run Examples

```bash
cd examples/java
javac -cp ../../target/fastcollection-1.0.0.jar *.java
java -cp ../../target/fastcollection-1.0.0.jar:. BasicExample
```

### Verify Build

```bash
jar tf target/fastcollection-1.0.0.jar | grep native
```

### Diagnostics

```java
import com.kuber.fastcollection.NativeLibraryLoader;
NativeLibraryLoader.printDiagnostics();
```

---

## Support

For build issues, please provide:
- OS and version: `uname -a` (Linux/macOS) or `systeminfo` (Windows)
- Java version: `java -version`
- Maven version: `mvn -version`
- CMake version: `cmake --version`
- Full build log: `mvn clean package -X 2>&1 | tee build.log`

**Contact**: ajsinha@gmail.com

---

**FastCollection v1.0.0** - Comprehensive Build Guide
