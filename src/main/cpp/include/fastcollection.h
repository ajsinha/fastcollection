/**
 * Copyright © 2025-2030, All Rights Reserved
 * Ashutosh Sinha | Email: ajsinha@gmail.com
 * 
 * Legal Notice: This module and the associated software architecture are proprietary
 * and confidential. Unauthorized copying, distribution, modification, or use is
 * strictly prohibited without explicit written permission from the copyright holder.
 * 
 * Patent Pending: Certain architectural patterns and implementations described in
 * this module may be subject to patent applications.
 * 
 * @file fastcollection.h
 * @brief Main header file for FastCollection library
 * 
 * FastCollection is an ultra high-performance, memory-mapped collections library
 * providing List, Set, Map, Queue, and Stack implementations with:
 * 
 * - Nanosecond-level operation latency
 * - Lock-free concurrent access where possible
 * - Memory-mapped persistence and inter-process communication
 * - JNI bindings for Java and pybind11 bindings for Python
 * 
 * Performance Targets:
 * - List operations: 100ns - 500ns
 * - Set/Map operations: 100ns - 400ns
 * - Queue/Stack operations: 50ns - 200ns
 * - Concurrent throughput: 5-10M+ ops/sec
 */

#ifndef FASTCOLLECTION_H
#define FASTCOLLECTION_H

// Version macros
#define FASTCOLLECTION_VERSION_MAJOR 1
#define FASTCOLLECTION_VERSION_MINOR 0
#define FASTCOLLECTION_VERSION_PATCH 0
#define FASTCOLLECTION_VERSION_STRING "1.0.0"

// Include all collection headers
#include "fc_common.h"
#include "fc_serialization.h"
#include "fc_list.h"
#include "fc_set.h"
#include "fc_map.h"
#include "fc_queue.h"
#include "fc_stack.h"

namespace fastcollection {

/**
 * @brief Library initialization (call once at startup)
 */
void initialize();

/**
 * @brief Library cleanup (call before exit)
 */
void shutdown();

/**
 * @brief Get library version string
 */
const char* version();

/**
 * @brief Delete a memory-mapped file and all associated data
 */
bool deleteCollectionFile(const std::string& filename);

/**
 * @brief Check if a memory-mapped file exists and is valid
 */
bool isValidCollectionFile(const std::string& filename);

/**
 * @brief Get statistics about a memory-mapped file
 */
struct FileStats {
    size_t total_size;
    size_t used_size;
    size_t free_size;
    uint32_t element_count;
    uint64_t created_at;
    uint64_t modified_at;
};
bool getFileStats(const std::string& filename, FileStats& stats);

} // namespace fastcollection

#endif // FASTCOLLECTION_H
