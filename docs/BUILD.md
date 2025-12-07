# FastCollection Build Guide

Copyright © 2025-2030, Ashutosh Sinha (ajsinha@gmail.com)  
Patent Pending

## Prerequisites

### Linux (Ubuntu/Debian)

```bash
# Build tools
sudo apt-get update
sudo apt-get install -y build-essential cmake maven openjdk-17-jdk

# Boost libraries
sudo apt-get install -y libboost-all-dev

# Python bindings (optional)
sudo apt-get install -y python3-dev python3-pip
pip3 install pybind11
```

### macOS

```bash
# Homebrew
brew install cmake boost maven openjdk@17

# Python bindings (optional)
pip3 install pybind11
```

### Windows

1. Install Visual Studio 2022 with C++ workload
2. Install CMake (https://cmake.org)
3. Install Boost (https://www.boost.org/users/download/)
4. Install JDK 17+ (https://adoptium.net)
5. Install Maven (https://maven.apache.org)

## Building

### Quick Build (Maven - Recommended)

```bash
# Full build: Java + Native
mvn clean package

# Skip tests
mvn clean package -DskipTests

# Build only Java (no native)
mvn clean package -Dskip.native.build=true
```

### Manual Build

#### Step 1: Build Native Library

```bash
cd src/main/cpp
mkdir build && cd build

# Configure
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build . --parallel

# Libraries will be in build/lib/
```

#### Step 2: Build Java

```bash
# From project root
mvn clean compile

# Package JAR
mvn package -DskipTests
```

### Build Options

| Option | Description |
|--------|-------------|
| `-DCMAKE_BUILD_TYPE=Release` | Optimized build |
| `-DCMAKE_BUILD_TYPE=Debug` | Debug build with sanitizers |
| `-DBUILD_TESTS=ON` | Build test executables |
| `-DJAVA_HOME=/path/to/jdk` | Custom JDK location |

## Running Tests

### C++ Tests

```bash
cd src/main/cpp/build

# Run all tests
ctest

# Run specific test
./lib/fc_test_list
./lib/fc_test_map
./lib/fc_benchmark 100000
```

### Java Tests

```bash
mvn test
```

### Python Tests

```bash
# Build Python module
pip install .

# Test
python -c "from fastcollection import FastList; print('OK')"
```

## Output Files

After successful build:

```
target/
├── fastcollection-1.0.0.jar           # Java classes
├── fastcollection-1.0.0-sources.jar   # Source JAR
├── fastcollection-1.0.0-javadoc.jar   # Javadoc JAR
└── classes/
    └── native/
        ├── linux-x86_64/
        │   └── libfastcollection.so
        ├── darwin-x86_64/
        │   └── libfastcollection.dylib
        └── windows-x86_64/
            └── fastcollection.dll
```

## IDE Setup

### IntelliJ IDEA

1. Open project folder
2. Import as Maven project
3. Set JDK 17+
4. Mark `src/main/java` as Sources Root
5. Mark `src/test/java` as Test Sources Root

### Eclipse

1. File → Import → Maven → Existing Maven Projects
2. Select project folder
3. Configure JDK 17+

### CLion (C++ Development)

1. Open `src/main/cpp` folder
2. CLion will detect CMakeLists.txt
3. Configure CMake profile

## Troubleshooting

### Boost Not Found

```bash
# Set Boost root explicitly
cmake .. -DBOOST_ROOT=/usr/local

# Or install to standard location
sudo apt-get install libboost-all-dev
```

### JNI Headers Not Found

```bash
# Set JAVA_HOME
export JAVA_HOME=/usr/lib/jvm/java-17-openjdk-amd64
cmake .. -DJAVA_HOME=$JAVA_HOME
```

### Native Library Load Error

```bash
# Check library exists
ls -la target/classes/native/linux-x86_64/

# Run with library path
java -Djava.library.path=./target/classes/native/linux-x86_64 -jar app.jar
```

## Performance Tuning

For maximum performance:

```bash
# Build with native optimizations
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DCMAKE_CXX_FLAGS="-O3 -march=native -mtune=native"

# Disable debug checks
cmake .. -DCMAKE_CXX_FLAGS="-DNDEBUG"
```

## Cross-Compilation

### Linux ARM64

```bash
cmake .. -DCMAKE_SYSTEM_NAME=Linux \
         -DCMAKE_SYSTEM_PROCESSOR=aarch64 \
         -DCMAKE_C_COMPILER=aarch64-linux-gnu-gcc \
         -DCMAKE_CXX_COMPILER=aarch64-linux-gnu-g++
```
