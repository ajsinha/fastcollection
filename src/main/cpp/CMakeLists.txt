# Copyright © 2025-2030, All Rights Reserved
# Ashutosh Sinha | Email: ajsinha@gmail.com
# Legal Notice: This module and the associated software architecture are proprietary and confidential.
# Unauthorized copying, distribution, modification, or use is strictly prohibited without explicit
# written permission from the copyright holder.
# Patent Pending: Certain architectural patterns and implementations described in this module may be
# subject to patent applications.

cmake_minimum_required(VERSION 3.18)
project(fastcollection VERSION 1.0.0 LANGUAGES CXX)

# =============================================================================
# Platform Detection
# =============================================================================
if(WIN32)
    set(FC_PLATFORM "windows")
    set(FC_LIB_PREFIX "")
    set(FC_LIB_SUFFIX ".dll")
    set(FC_STATIC_SUFFIX ".lib")
elseif(APPLE)
    set(FC_PLATFORM "macos")
    set(FC_LIB_PREFIX "lib")
    set(FC_LIB_SUFFIX ".dylib")
    set(FC_STATIC_SUFFIX ".a")
else()
    set(FC_PLATFORM "linux")
    set(FC_LIB_PREFIX "lib")
    set(FC_LIB_SUFFIX ".so")
    set(FC_STATIC_SUFFIX ".a")
endif()

# Architecture detection
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(FC_ARCH "x64")
else()
    set(FC_ARCH "x86")
endif()

# ARM detection
if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm|ARM|aarch64|AARCH64")
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(FC_ARCH "arm64")
    else()
        set(FC_ARCH "arm")
    endif()
endif()

message(STATUS "Platform: ${FC_PLATFORM}")
message(STATUS "Architecture: ${FC_ARCH}")

# =============================================================================
# C++ Standard and Common Settings
# =============================================================================
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# =============================================================================
# Compiler Flags (Platform-Specific)
# =============================================================================
if(MSVC)
    # Windows/MSVC flags
    set(CMAKE_CXX_FLAGS_RELEASE "/O2 /DNDEBUG /GL /MT")
    set(CMAKE_CXX_FLAGS_DEBUG "/Od /DEBUG /MTd")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "/LTCG")
    set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "/LTCG")
    # Static runtime linkage
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
elseif(APPLE)
    # macOS/Clang flags
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -ffast-math -funroll-loops")
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DDEBUG")
    # For Apple Silicon and Intel
    if(FC_ARCH STREQUAL "arm64")
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -mcpu=apple-m1")
    else()
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -march=native -mtune=native")
    endif()
else()
    # Linux/GCC flags
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -march=native -mtune=native -ffast-math -funroll-loops")
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DDEBUG -fsanitize=address,undefined")
    # Static linking flags for Linux
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")
endif()

# =============================================================================
# Output Directory Configuration
# =============================================================================
if(NOT DEFINED OUTPUT_DIR)
    set(OUTPUT_DIR ${CMAKE_BINARY_DIR}/lib/${FC_PLATFORM}-${FC_ARCH})
endif()

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${OUTPUT_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${OUTPUT_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_DIR})

# Multi-config generators (Visual Studio, Xcode)
foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${OUTPUT_DIR})
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${OUTPUT_DIR})
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${OUTPUT_DIR})
endforeach()

# =============================================================================
# Find Dependencies
# =============================================================================

# Threads
find_package(Threads REQUIRED)

# Boost configuration - try static first, fall back to shared
set(Boost_USE_MULTITHREADED ON)

# Try to find static Boost libraries first
set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_STATIC_RUNTIME OFF)

find_package(Boost 1.74 QUIET COMPONENTS system thread)

if(NOT Boost_FOUND)
    # Fall back to shared libraries
    message(STATUS "Static Boost not found, trying shared libraries...")
    set(Boost_USE_STATIC_LIBS OFF)
    set(Boost_USE_STATIC_RUNTIME OFF)
    find_package(Boost 1.74 REQUIRED COMPONENTS system thread)
endif()

if(Boost_FOUND)
    message(STATUS "Boost found: ${Boost_VERSION}")
    message(STATUS "Boost libraries: ${Boost_LIBRARIES}")
    message(STATUS "Boost static libs: ${Boost_USE_STATIC_LIBS}")
    include_directories(${Boost_INCLUDE_DIRS})
else()
    message(FATAL_ERROR "Boost not found. Please install Boost with interprocess support.")
endif()

# JNI
if(DEFINED JAVA_HOME)
    set(ENV{JAVA_HOME} ${JAVA_HOME})
endif()
find_package(JNI REQUIRED)
if(JNI_FOUND)
    message(STATUS "JNI found: ${JNI_INCLUDE_DIRS}")
    include_directories(${JNI_INCLUDE_DIRS})
else()
    message(FATAL_ERROR "JNI not found. Please set JAVA_HOME.")
endif()

# Python (optional)
find_package(Python3 COMPONENTS Interpreter Development)
if(Python3_FOUND)
    message(STATUS "Python found: ${Python3_VERSION}")
    find_package(pybind11 QUIET)
endif()

# =============================================================================
# Include Directories
# =============================================================================
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/generated
    ${CMAKE_CURRENT_SOURCE_DIR}/src/jni
)

# =============================================================================
# Source Files
# =============================================================================
set(CORE_SOURCES
    src/fc_common.cpp
    src/fc_list.cpp
    src/fc_set.cpp
    src/fc_map.cpp
    src/fc_queue.cpp
    src/fc_stack.cpp
)

set(JNI_SOURCES
    src/jni/jni_list.cpp
    src/jni/jni_collections.cpp
)

# =============================================================================
# Core Static Library (internal use)
# =============================================================================
add_library(fastcollection_core STATIC ${CORE_SOURCES})
target_link_libraries(fastcollection_core
    PUBLIC
        Threads::Threads
        ${Boost_LIBRARIES}
)
target_compile_definitions(fastcollection_core PRIVATE FC_BUILDING_LIBRARY)

# =============================================================================
# JNI Shared Library (self-contained, statically linked)
# =============================================================================
add_library(fastcollection_jni SHARED ${CORE_SOURCES} ${JNI_SOURCES})

# Platform-specific linking for self-contained library
if(WIN32)
    target_link_libraries(fastcollection_jni
        PRIVATE
            ${Boost_LIBRARIES}
            ${JNI_LIBRARIES}
            Threads::Threads
    )
    # Link runtime statically on Windows
    set_target_properties(fastcollection_jni PROPERTIES
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
    )
elseif(APPLE)
    target_link_libraries(fastcollection_jni
        PRIVATE
            ${Boost_LIBRARIES}
            ${JNI_LIBRARIES}
            Threads::Threads
            "-framework CoreFoundation"
    )
    # Ensure all symbols are resolved
    set_target_properties(fastcollection_jni PROPERTIES
        LINK_FLAGS "-Wl,-all_load"
    )
else()
    # Linux linking
    if(Boost_USE_STATIC_LIBS)
        # Static Boost - use whole-archive
        target_link_libraries(fastcollection_jni
            PRIVATE
                -Wl,--whole-archive
                ${Boost_LIBRARIES}
                -Wl,--no-whole-archive
                ${JNI_LIBRARIES}
                Threads::Threads
                -static-libgcc
                -static-libstdc++
                rt
                dl
        )
    else()
        # Shared Boost - link normally
        target_link_libraries(fastcollection_jni
            PRIVATE
                ${Boost_LIBRARIES}
                ${JNI_LIBRARIES}
                Threads::Threads
                -static-libgcc
                -static-libstdc++
                rt
                dl
        )
    endif()
    # Hide internal symbols
    set_target_properties(fastcollection_jni PROPERTIES
        LINK_FLAGS "-Wl,--exclude-libs,ALL -Wl,-z,defs"
    )
endif()

# Output naming: fastcollection-<platform>-<arch>.<ext>
set(FC_OUTPUT_NAME "fastcollection-${FC_PLATFORM}-${FC_ARCH}")

set_target_properties(fastcollection_jni PROPERTIES
    OUTPUT_NAME "${FC_OUTPUT_NAME}"
    PREFIX ""
)

# Platform-specific suffix
if(APPLE)
    set_target_properties(fastcollection_jni PROPERTIES SUFFIX ".dylib")
elseif(WIN32)
    set_target_properties(fastcollection_jni PROPERTIES SUFFIX ".dll")
else()
    set_target_properties(fastcollection_jni PROPERTIES SUFFIX ".so")
endif()

target_compile_definitions(fastcollection_jni PRIVATE 
    FC_BUILDING_LIBRARY
    FC_PLATFORM_${FC_PLATFORM}
    FC_ARCH_${FC_ARCH}
)

# =============================================================================
# Python Bindings (if available)
# =============================================================================
if(pybind11_FOUND AND Python3_FOUND)
    pybind11_add_module(pyfastcollection src/python/pyfastcollection.cpp)
    target_link_libraries(pyfastcollection PRIVATE fastcollection_core)
    set_target_properties(pyfastcollection PROPERTIES
        OUTPUT_NAME "pyfastcollection"
    )
    message(STATUS "Python bindings will be built")
else()
    message(STATUS "Python bindings skipped (pybind11 not found)")
endif()

# =============================================================================
# Standalone C++ Shared Library
# =============================================================================
add_library(fastcollection_shared SHARED ${CORE_SOURCES})

if(WIN32)
    target_link_libraries(fastcollection_shared
        PUBLIC
            ${Boost_LIBRARIES}
            Threads::Threads
    )
elseif(APPLE)
    target_link_libraries(fastcollection_shared
        PUBLIC
            ${Boost_LIBRARIES}
            Threads::Threads
    )
else()
    target_link_libraries(fastcollection_shared
        PUBLIC
            ${Boost_LIBRARIES}
            Threads::Threads
            -static-libgcc
            -static-libstdc++
            rt
    )
endif()

set_target_properties(fastcollection_shared PROPERTIES
    OUTPUT_NAME "fastcollection_cpp-${FC_PLATFORM}-${FC_ARCH}"
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
)

# =============================================================================
# Install Targets
# =============================================================================
install(TARGETS fastcollection_core fastcollection_jni fastcollection_shared
    LIBRARY DESTINATION lib/${FC_PLATFORM}-${FC_ARCH}
    ARCHIVE DESTINATION lib/${FC_PLATFORM}-${FC_ARCH}
    RUNTIME DESTINATION bin/${FC_PLATFORM}-${FC_ARCH}
)

install(DIRECTORY include/
    DESTINATION include/fastcollection
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)

# =============================================================================
# Tests
# =============================================================================
option(BUILD_TESTS "Build test executables" ON)
if(BUILD_TESTS)
    enable_testing()
    
    add_executable(fc_test_list test/test_list.cpp)
    target_link_libraries(fc_test_list fastcollection_core)
    add_test(NAME TestList COMMAND fc_test_list)
    
    add_executable(fc_test_map test/test_map.cpp)
    target_link_libraries(fc_test_map fastcollection_core)
    add_test(NAME TestMap COMMAND fc_test_map)
    
    add_executable(fc_benchmark test/benchmark.cpp)
    target_link_libraries(fc_benchmark fastcollection_core)
endif()

# =============================================================================
# Copy library to resources directory for JAR packaging
# =============================================================================
set(RESOURCES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../resources/native/${FC_PLATFORM}/${FC_ARCH})
add_custom_command(TARGET fastcollection_jni POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${RESOURCES_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:fastcollection_jni> ${RESOURCES_DIR}/
    COMMENT "Copying native library to resources: ${RESOURCES_DIR}"
)

# =============================================================================
# Configuration Summary
# =============================================================================
message(STATUS "")
message(STATUS "========================================")
message(STATUS "FastCollection Configuration Summary")
message(STATUS "========================================")
message(STATUS "  Platform:        ${FC_PLATFORM}")
message(STATUS "  Architecture:    ${FC_ARCH}")
message(STATUS "  Build type:      ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ Standard:    ${CMAKE_CXX_STANDARD}")
message(STATUS "  Output dir:      ${OUTPUT_DIR}")
message(STATUS "  Library name:    ${FC_OUTPUT_NAME}${FC_LIB_SUFFIX}")
message(STATUS "  Boost version:   ${Boost_VERSION}")
message(STATUS "  Boost static:    ${Boost_USE_STATIC_LIBS}")
message(STATUS "  JNI include:     ${JNI_INCLUDE_DIRS}")
if(Python3_FOUND AND pybind11_FOUND)
    message(STATUS "  Python bindings: YES")
else()
    message(STATUS "  Python bindings: NO")
endif()
message(STATUS "========================================")
message(STATUS "")
