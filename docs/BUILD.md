# FastCollection v1.0.0 Build Guide

## Overview

FastCollection uses a hybrid build system:
- **Maven** for Java compilation, packaging, and orchestration
- **CMake** for native C++ code compilation

The build system automatically detects your platform and produces a native library. When static Boost libraries are available, the build produces a fully **self-contained** library with no runtime dependencies. When only shared Boost libraries are available, the library will link against them dynamically.

## Supported Platforms

| Platform | Architecture | Compiler | Library Extension |
|----------|--------------|----------|-------------------|
| Linux    | x64          | GCC 10+  | `.so` |
| Linux    | x86          | GCC 10+  | `.so` |
| Linux    | arm64        | GCC 10+  | `.so` |
| Linux    | arm          | GCC 10+  | `.so` |
| macOS    | x64 (Intel)  | Clang 13+ | `.dylib` |
| macOS    | arm64 (Apple Silicon) | Clang 13+ | `.dylib` |
| Windows  | x64          | MSVC 2022 | `.dll` |
| Windows  | x86          | MSVC 2022 | `.dll` |
| Windows  | arm64        | MSVC 2022 | `.dll` |

## Prerequisites

### All Platforms

- **Java JDK 17+** with `JAVA_HOME` environment variable set
- **Maven 3.8+**
- **CMake 3.18+**
- **C++20 compatible compiler**
- **Boost 1.74+** (development headers; static or shared libraries)

### Linux (Ubuntu/Debian)

```bash
# Update package list
sudo apt-get update

# Install build tools
sudo apt-get install -y \
    build-essential \
    cmake \
    libboost-all-dev \
    openjdk-17-jdk

# Set JAVA_HOME
export JAVA_HOME=/usr/lib/jvm/java-17-openjdk-amd64
echo 'export JAVA_HOME=/usr/lib/jvm/java-17-openjdk-amd64' >> ~/.bashrc

# Verify installations
java -version
mvn -version
cmake --version
g++ --version
```

### Linux (RHEL/CentOS/Fedora)

```bash
# Install build tools
sudo dnf install -y \
    gcc-c++ \
    cmake \
    boost-devel \
    boost-static \
    java-17-openjdk-devel

# Set JAVA_HOME
export JAVA_HOME=/usr/lib/jvm/java-17-openjdk
```

### Linux (Arch Linux)

```bash
sudo pacman -S base-devel cmake boost jdk17-openjdk maven
export JAVA_HOME=/usr/lib/jvm/java-17-openjdk
```

### macOS

```bash
# Install Xcode command line tools
xcode-select --install

# Install Homebrew (if not installed)
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install dependencies
brew install cmake boost maven

# Install Java (if not installed)
brew install openjdk@17

# Set JAVA_HOME
export JAVA_HOME=$(/usr/libexec/java_home -v 17)
echo 'export JAVA_HOME=$(/usr/libexec/java_home -v 17)' >> ~/.zshrc

# Verify
java -version
mvn -version
cmake --version
```

### Windows

1. **Install Visual Studio 2022**
   - Download from https://visualstudio.microsoft.com/
   - Select "Desktop development with C++" workload
   - Ensure "C++ CMake tools" is selected

2. **Install CMake** (if not included with VS)
   - Download from https://cmake.org/download/
   - Add to PATH during installation

3. **Install Boost**

   Option A: Using vcpkg (Recommended)
   ```powershell
   # Install vcpkg
   git clone https://github.com/Microsoft/vcpkg.git
   cd vcpkg
   .\bootstrap-vcpkg.bat
   
   # Install Boost static libraries
   .\vcpkg install boost:x64-windows-static
   
   # Set environment variable
   set BOOST_ROOT=C:\vcpkg\installed\x64-windows-static
   ```

   Option B: Pre-built binaries
   - Download from https://www.boost.org/users/download/
   - Extract to `C:\boost_1_83_0`
   - Set `BOOST_ROOT=C:\boost_1_83_0`

4. **Install JDK 17**
   - Download from https://adoptium.net/
   - Set `JAVA_HOME=C:\Program Files\Eclipse Adoptium\jdk-17.x.x`

5. **Install Maven**
   - Download from https://maven.apache.org/download.cgi
   - Extract and add `bin` folder to PATH

## Building

### Quick Build (Single Platform)

```bash
# Clone repository
git clone https://github.com/abhikarta/fastcollection.git
cd fastcollection

# Build (auto-detects platform)
mvn clean package

# Output JAR
ls -la target/fastcollection-1.0.0.jar

# Native library location
ls -la target/native/
```

### Build Options

```bash
# Release build (default, optimized)
mvn clean package

# Debug build (with symbols, sanitizers on Linux)
mvn clean package -Pdebug

# Skip tests
mvn clean package -DskipTests

# Skip native compilation (Java only)
mvn clean package -Pskip-native

# Release build with source and javadoc JARs
mvn clean install -Prelease

# Verbose output
mvn clean package -X

# Parallel build
mvn clean package -T 4
```

### Build Profiles

| Profile | Command | Description |
|---------|---------|-------------|
| (default) | `mvn clean package` | Standard build with native compilation |
| `debug` | `mvn clean package -Pdebug` | Debug build with symbols |
| `release` | `mvn clean install -Prelease` | Generates source and javadoc JARs |
| `skip-native` | `mvn clean package -Pskip-native` | Skip native compilation |
| `multi-platform` | `mvn clean package -Pmulti-platform` | Build with all pre-compiled natives |
| `python` | `mvn clean package -Ppython` | Also build Python bindings |

### Verify Platform Detection

```bash
# Show active Maven profiles
mvn help:active-profiles

# Expected output example:
# - linux-x64 (auto-activated by OS)
# or
# - macos-arm64 (auto-activated by OS)
# or
# - windows-x64 (auto-activated by OS)
```

## Multi-Platform JAR

To create a single JAR containing native libraries for all platforms, you need to build on each platform first, then combine them.

### Step 1: Build on Each Platform

**On Linux x64:**
```bash
mvn clean package -DskipTests
mkdir -p collected-natives/linux/x64
cp target/native/linux/x64/*.so collected-natives/linux/x64/
```

**On Linux arm64:**
```bash
mvn clean package -DskipTests
mkdir -p collected-natives/linux/arm64
cp target/native/linux/arm64/*.so collected-natives/linux/arm64/
```

**On macOS x64 (Intel):**
```bash
mvn clean package -DskipTests
mkdir -p collected-natives/macos/x64
cp target/native/macos/x64/*.dylib collected-natives/macos/x64/
```

**On macOS arm64 (Apple Silicon):**
```bash
mvn clean package -DskipTests
mkdir -p collected-natives/macos/arm64
cp target/native/macos/arm64/*.dylib collected-natives/macos/arm64/
```

**On Windows x64:**
```powershell
mvn clean package -DskipTests
mkdir collected-natives\windows\x64
copy target\native\windows\x64\*.dll collected-natives\windows\x64\
```

### Step 2: Collect Libraries

Transfer all libraries to a single machine:

```
src/main/resources/native/
├── linux/
│   ├── x64/
│   │   └── fastcollection-linux-x64.so
│   ├── x86/
│   │   └── fastcollection-linux-x86.so
│   └── arm64/
│       └── fastcollection-linux-arm64.so
├── macos/
│   ├── x64/
│   │   └── fastcollection-macos-x64.dylib
│   └── arm64/
│       └── fastcollection-macos-arm64.dylib
└── windows/
    ├── x64/
    │   └── fastcollection-windows-x64.dll
    └── x86/
        └── fastcollection-windows-x86.dll
```

### Step 3: Package Multi-Platform JAR

```bash
# Build with multi-platform profile (skips native compilation)
mvn clean package -Pmulti-platform -Pskip-native

# Verify JAR contents
jar tf target/fastcollection-1.0.0.jar | grep native
# Should show:
# native/linux/x64/fastcollection-linux-x64.so
# native/macos/arm64/fastcollection-macos-arm64.dylib
# native/windows/x64/fastcollection-windows-x64.dll
# etc.
```

## CI/CD Multi-Platform Build

### GitHub Actions

```yaml
name: Build Multi-Platform

on:
  push:
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
      run: sudo apt-get install -y libboost-all-dev
    
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
    
    - name: Upload native library
      uses: actions/upload-artifact@v4
      with:
        name: native-${{ matrix.platform }}-${{ matrix.arch }}
        path: target/native/

  package:
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
      run: mvn clean package -Pmulti-platform -Pskip-native
    
    - name: Upload JAR
      uses: actions/upload-artifact@v4
      with:
        name: fastcollection-1.0.0
        path: target/fastcollection-1.0.0.jar
```

## Static Linking Details

FastCollection native libraries are statically linked with:

| Component | Linux | macOS | Windows |
|-----------|-------|-------|---------|
| Boost.Interprocess | ✅ Static | ✅ Static | ✅ Static |
| Boost.System | ✅ Static | ✅ Static | ✅ Static |
| Boost.Thread | ✅ Static | ✅ Static | ✅ Static |
| C++ Runtime | ✅ Static (`-static-libstdc++`) | ⚡ System | ✅ Static (`/MT`) |
| C Runtime | ✅ Static (`-static-libgcc`) | ⚡ System | ✅ Static (`/MT`) |
| pthread | ⚡ Dynamic | ⚡ System | N/A |

**Result**: The native library is self-contained and only requires:
- Linux: libc, libpthread, librt, libdl (always present)
- macOS: System frameworks (always present)
- Windows: kernel32.dll, ntdll.dll (always present)

## Running Tests

```bash
# Run all tests
mvn test

# Run specific test class
mvn test -Dtest=FastCollectionListTest

# Run with custom library path
mvn test -Djava.library.path=target/native/linux/x64

# Run tests with debug output
mvn test -Dfastcollection.debug=true
```

## Troubleshooting

### CMake Not Found

```bash
# Check CMake installation
cmake --version

# On Windows, ensure CMake is in PATH or use full path
"C:\Program Files\CMake\bin\cmake.exe" --version
```

### Boost Not Found

```bash
# Linux: Check Boost installation
dpkg -l | grep libboost

# Set BOOST_ROOT if needed
export BOOST_ROOT=/usr/local
mvn clean package

# macOS: Check Homebrew Boost
brew info boost
export BOOST_ROOT=$(brew --prefix boost)

# Windows: Verify vcpkg integration
vcpkg list | findstr boost
```

### JNI Headers Not Found

```bash
# Ensure JAVA_HOME points to JDK (not JRE)
echo $JAVA_HOME
ls $JAVA_HOME/include/jni.h

# If missing, install JDK
# Ubuntu: sudo apt install openjdk-17-jdk
# macOS: brew install openjdk@17
```

### Library Load Errors at Runtime

```java
// Add diagnostic code to your application
import com.abhikarta.fastcollection.NativeLibraryLoader;

public class Debug {
    public static void main(String[] args) {
        NativeLibraryLoader.printDiagnostics();
    }
}
```

Output shows:
- Detected platform and architecture
- Expected library names
- Searched paths
- JAR resource locations

### Windows Visual Studio Version

If CMake can't find Visual Studio:

```powershell
# Specify generator explicitly
mvn clean package -Dcmake.generator="Visual Studio 17 2022"

# For older VS versions
mvn clean package -Dcmake.generator="Visual Studio 16 2019"
```

### macOS Code Signing

For distribution, sign the native library:

```bash
codesign --sign "Developer ID Application: Your Name" \
    target/native/macos/arm64/fastcollection-macos-arm64.dylib
```

### Linux Symbol Visibility

If you get "undefined symbol" errors:

```bash
# Check exported symbols
nm -D target/native/linux/x64/fastcollection-linux-x64.so | grep Java_
```

All JNI functions should be visible (T = text/code section).

## Performance Optimization

### Release Build Flags

The release build (`-Pcmake.build.type=Release`) enables:

| Platform | Optimization Flags |
|----------|-------------------|
| Linux/GCC | `-O3 -march=native -mtune=native -ffast-math -funroll-loops` |
| macOS/Clang | `-O3 -ffast-math -funroll-loops` |
| macOS/arm64 | `-O3 -mcpu=apple-m1 -ffast-math` |
| Windows/MSVC | `/O2 /GL /LTCG` |

### Portable Build

For maximum compatibility (at slight performance cost):

```bash
# Edit CMakeLists.txt to use -march=x86-64 instead of -march=native
# Or set via command line:
mvn clean package -Dcmake.cxx.flags="-O3 -march=x86-64"
```

## Support

For build issues, please include:
- Operating system and version (`uname -a` or `systeminfo`)
- Java version (`java -version`)
- CMake version (`cmake --version`)
- Compiler version (`gcc --version`, `clang --version`, or `cl`)
- Full build log (`mvn clean package -X 2>&1 | tee build.log`)

Contact: ajsinha@gmail.com

---

**FastCollection v1.0.0** - Build Guide
