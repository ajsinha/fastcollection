# Copyright © 2025-2030, All Rights Reserved
# Ashutosh Sinha | Email: ajsinha@gmail.com
# Legal Notice: This module and the associated software architecture are proprietary and confidential.
# Unauthorized copying, distribution, modification, or use is strictly prohibited without explicit
# written permission from the copyright holder.
# Patent Pending: Certain architectural patterns and implementations described in this module may be
# subject to patent applications.

cmake_minimum_required(VERSION 3.18)
project(fastcollection VERSION 1.0.0 LANGUAGES CXX)

# C++20 for advanced features
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler flags for maximum performance
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -march=native -mtune=native -ffast-math -funroll-loops")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DDEBUG -fsanitize=address,undefined")

# Output directory
if(NOT DEFINED OUTPUT_DIR)
    set(OUTPUT_DIR ${CMAKE_BINARY_DIR}/lib)
endif()

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${OUTPUT_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${OUTPUT_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_DIR})

# Find required packages
find_package(Threads REQUIRED)

# Find Boost with interprocess support
find_package(Boost 1.74 REQUIRED COMPONENTS system thread)
if(Boost_FOUND)
    message(STATUS "Boost found: ${Boost_VERSION}")
    include_directories(${Boost_INCLUDE_DIRS})
else()
    message(FATAL_ERROR "Boost not found. Please install Boost with interprocess support.")
endif()

# Find JNI
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

# Find Python for Python bindings (optional)
find_package(Python3 COMPONENTS Interpreter Development)
if(Python3_FOUND)
    message(STATUS "Python found: ${Python3_VERSION}")
    find_package(pybind11 QUIET)
endif()

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/generated
    ${CMAKE_CURRENT_SOURCE_DIR}/src/jni
)

# Source files for core library
set(CORE_SOURCES
    src/fc_common.cpp
    src/fc_list.cpp
    src/fc_set.cpp
    src/fc_map.cpp
    src/fc_queue.cpp
    src/fc_stack.cpp
)

# JNI source files
set(JNI_SOURCES
    src/jni/jni_list.cpp
    src/jni/jni_collections.cpp
)

# Create core static library
add_library(fastcollection_core STATIC ${CORE_SOURCES})
target_link_libraries(fastcollection_core
    PUBLIC
        Threads::Threads
        ${Boost_LIBRARIES}
)
target_compile_definitions(fastcollection_core PRIVATE FC_BUILDING_LIBRARY)

# Create JNI shared library
add_library(fastcollection_jni SHARED ${JNI_SOURCES})
target_link_libraries(fastcollection_jni
    PRIVATE
        fastcollection_core
        ${JNI_LIBRARIES}
)
set_target_properties(fastcollection_jni PROPERTIES
    OUTPUT_NAME "fastcollection"
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

# Python bindings (if pybind11 found)
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

# Standalone C++ shared library for direct use
add_library(fastcollection_shared SHARED ${CORE_SOURCES})
target_link_libraries(fastcollection_shared
    PUBLIC
        Threads::Threads
        ${Boost_LIBRARIES}
)
set_target_properties(fastcollection_shared PROPERTIES
    OUTPUT_NAME "fastcollection_cpp"
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
)

# Install targets
install(TARGETS fastcollection_core fastcollection_jni fastcollection_shared
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/
    DESTINATION include/fastcollection
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)

# Tests
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

# Summary
message(STATUS "")
message(STATUS "FastCollection Configuration Summary:")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Output directory: ${OUTPUT_DIR}")
message(STATUS "  Boost version: ${Boost_VERSION}")
message(STATUS "  JNI include: ${JNI_INCLUDE_DIRS}")
if(Python3_FOUND AND pybind11_FOUND)
    message(STATUS "  Python bindings: YES")
else()
    message(STATUS "  Python bindings: NO")
endif()
message(STATUS "")
