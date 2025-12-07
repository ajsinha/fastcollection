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
 * @file fc_set.h
 * @brief High-performance memory-mapped set implementation with TTL support
 * 
 * ============================================================================
 * FASTCOLLECTION SET - MEMORY-MAPPED HASH SET
 * ============================================================================
 * 
 * OVERVIEW:
 * ---------
 * FastSet is an ultra high-performance hash set implementation that stores data
 * in memory-mapped files. It provides O(1) average-case operations for add,
 * remove, and contains, with automatic element expiration via TTL.
 * 
 * KEY FEATURES:
 * 1. PERSISTENCE: Set data survives process restarts
 * 2. IPC: Multiple processes can share the same set
 * 3. TTL: Elements auto-expire after configurable duration
 * 4. LOCK-STRIPED: Per-bucket locking for high concurrency
 * 5. CACHE-OPTIMIZED: 64-byte aligned entries prevent false sharing
 * 
 * HASH TABLE ARCHITECTURE:
 * ------------------------
 * The set uses open addressing with chaining within buckets:
 * 
 * +------------------+
 * | HashTableHeader  |  <- Bucket count, load factor, statistics
 * +------------------+
 * | Bucket[0]        |  -> Node -> Node -> Node (chain)
 * +------------------+
 * | Bucket[1]        |  -> Node -> Node
 * +------------------+
 * | ...              |
 * +------------------+
 * | Bucket[N-1]      |  -> Node
 * +------------------+
 * 
 * Each bucket has its own mutex, allowing concurrent operations on different
 * buckets. The default bucket count is 16384 (power of 2 for fast modulo).
 * 
 * TTL (TIME-TO-LIVE) FEATURE:
 * ---------------------------
 * Elements can be added with an expiration time:
 * 
 * - ttl_seconds = -1 (default): Element never expires
 * - ttl_seconds = 0: Element expires immediately
 * - ttl_seconds > 0: Element expires after specified seconds
 * 
 * Expired elements:
 * - Return false for contains() checks
 * - Are lazily cleaned during other operations
 * - Do not affect uniqueness (can add same value again after expiry)
 * 
 * USE CASES:
 * ----------
 * 1. RATE LIMITING: Track API calls with 60-second TTL
 *    set.add(clientId, 60);  // Expires in 1 minute
 *    
 * 2. DEDUPLICATION: Track processed IDs with 24-hour TTL
 *    if (!set.contains(messageId)) {
 *        set.add(messageId, 86400);  // 24-hour TTL
 *        processMessage(message);
 *    }
 *    
 * 3. SESSION TRACKING: Track active sessions with timeout
 *    set.add(sessionId, 1800);  // 30-minute session timeout
 * 
 * PERFORMANCE CHARACTERISTICS:
 * ----------------------------
 * Operation     | Average Time | Worst Case | Notes
 * --------------|--------------|------------|---------------------------
 * add           | < 300ns      | O(n/m)     | n=elements, m=buckets
 * remove        | < 300ns      | O(n/m)     | 
 * contains      | < 100ns      | O(n/m)     | Lock-free read path
 * clear         | O(n)         | O(n)       | Requires global lock
 * size          | < 50ns       | O(1)       | Atomic counter
 * 
 * Concurrent throughput: > 5M ops/sec with 8 threads
 * 
 * USAGE EXAMPLES:
 * ---------------
 * 
 * C++:
 *   FastSet set("/tmp/myset.fc", 64*1024*1024, true);
 *   
 *   // Add with TTL
 *   std::string id = "user123";
 *   set.add((uint8_t*)id.data(), id.size(), 300);  // 5-minute TTL
 *   
 *   // Check existence
 *   if (set.contains((uint8_t*)id.data(), id.size())) {
 *       // Element exists and not expired
 *   }
 * 
 * Java (via JNI):
 *   FastCollectionSet set = new FastCollectionSet("/tmp/myset.fc");
 *   set.add(userId, 300);  // 5-minute TTL
 *   if (set.contains(userId)) { ... }
 * 
 * Python (via pybind11):
 *   s = FastSet("/tmp/myset.fc")
 *   s.add(user_id, ttl=300)
 *   if user_id in s: ...
 */

#ifndef FASTCOLLECTION_SET_H
#define FASTCOLLECTION_SET_H

#include "fc_common.h"
#include "fc_serialization.h"
#include <functional>
#include <vector>

namespace fastcollection {

/**
 * @brief Ultra high-performance memory-mapped hash set with TTL support
 * 
 * Features:
 * - O(1) average add, remove, contains operations
 * - TTL support for automatic element expiration
 * - Lock-striped concurrency (per-bucket locking)
 * - Lock-free reads via optimistic concurrency
 * - Memory-mapped backing for persistence and IPC
 * 
 * Performance targets:
 * - Add: < 300ns average
 * - Remove: < 300ns average
 * - Contains: < 100ns average
 * - Concurrent throughput: > 5M ops/sec with 8 threads
 */
class FastSet {
public:
    /**
     * @brief Construct a FastSet with the given memory-mapped file
     * 
     * @param mmap_file Path to the memory-mapped file
     * @param initial_size Initial size of the memory-mapped region
     * @param create_new If true, create a new file (truncating any existing)
     * @param bucket_count Number of hash buckets (default 16384, must be power of 2)
     * 
     * @throws FastCollectionException if file cannot be created/opened
     */
    FastSet(const std::string& mmap_file,
            size_t initial_size = DEFAULT_INITIAL_SIZE,
            bool create_new = false,
            uint32_t bucket_count = HashTableHeader::DEFAULT_BUCKET_COUNT);
    
    ~FastSet();
    
    // Non-copyable
    FastSet(const FastSet&) = delete;
    FastSet& operator=(const FastSet&) = delete;
    
    // Movable
    FastSet(FastSet&&) noexcept;
    FastSet& operator=(FastSet&&) noexcept;
    
    // =========================================================================
    // CORE SET OPERATIONS
    // =========================================================================
    
    /**
     * @brief Add an element to the set
     * 
     * @param data Pointer to serialized element data
     * @param size Size of the data in bytes
     * @param ttl_seconds Time-to-live in seconds (-1 for infinite)
     * @return true if element was added, false if already exists
     * 
     * Time Complexity: O(1) average, O(n/m) worst case
     * 
     * If element exists but is expired, it will be replaced with new TTL.
     */
    bool add(const uint8_t* data, size_t size, int32_t ttl_seconds = TTL_INFINITE);
    
    /**
     * @brief Remove an element from the set
     * 
     * @param data Pointer to element data to remove
     * @param size Size of the data
     * @return true if element was found and removed
     * 
     * Time Complexity: O(1) average
     */
    bool remove(const uint8_t* data, size_t size);
    
    /**
     * @brief Check if set contains an element
     * 
     * @param data Pointer to element data to check
     * @param size Size of the data
     * @return true if element exists and is not expired
     * 
     * Time Complexity: O(1) average
     * 
     * This operation uses a lock-free read path for maximum performance.
     */
    bool contains(const uint8_t* data, size_t size) const;
    
    /**
     * @brief Get remaining TTL for an element
     * 
     * @param data Pointer to element data
     * @param size Size of the data
     * @return Remaining seconds (-1 if infinite, 0 if expired or not found)
     */
    int64_t getTTL(const uint8_t* data, size_t size) const;
    
    /**
     * @brief Update TTL for an existing element
     * 
     * @param data Pointer to element data
     * @param size Size of the data
     * @param ttl_seconds New TTL in seconds (-1 for infinite)
     * @return true if element was found and TTL updated
     */
    bool setTTL(const uint8_t* data, size_t size, int32_t ttl_seconds);
    
    // =========================================================================
    // BULK OPERATIONS
    // =========================================================================
    
    /**
     * @brief Add multiple elements to the set
     * 
     * @param elements Vector of (data, size, ttl) tuples
     * @return Number of elements actually added (excludes duplicates)
     */
    size_t addAll(const std::vector<std::tuple<const uint8_t*, size_t, int32_t>>& elements);
    
    /**
     * @brief Remove multiple elements from the set
     * 
     * @param elements Vector of (data, size) pairs
     * @return Number of elements actually removed
     */
    size_t removeAll(const std::vector<std::pair<const uint8_t*, size_t>>& elements);
    
    /**
     * @brief Retain only elements that satisfy a predicate
     * 
     * @param predicate Function returning true for elements to keep
     * @return Number of elements removed
     */
    size_t retainIf(std::function<bool(const uint8_t* data, size_t size)> predicate);
    
    /**
     * @brief Remove all expired elements
     * 
     * @return Number of elements removed
     */
    size_t removeExpired();
    
    // =========================================================================
    // ITERATION
    // =========================================================================
    
    /**
     * @brief Iterate over all non-expired elements
     * 
     * @param callback Function called for each element. Return false to stop.
     */
    void forEach(std::function<bool(const uint8_t* data, size_t size)> callback) const;
    
    /**
     * @brief Iterate with TTL information
     * 
     * @param callback Function receiving data, size, and remaining TTL
     */
    void forEachWithTTL(std::function<bool(const uint8_t* data, size_t size, 
                                           int64_t ttl_remaining)> callback) const;
    
    /**
     * @brief Copy all elements to a vector
     * 
     * @return Vector of element data copies
     */
    std::vector<std::vector<uint8_t>> toArray() const;
    
    // =========================================================================
    // UTILITY
    // =========================================================================
    
    /**
     * @brief Clear all elements from the set
     */
    void clear();
    
    /**
     * @brief Get the number of non-expired elements
     */
    size_t size() const;
    
    /**
     * @brief Check if set is empty
     */
    bool isEmpty() const;
    
    /**
     * @brief Get collection statistics
     */
    const CollectionStats& stats() const { return stats_; }
    
    /**
     * @brief Get the backing file path
     */
    const std::string& filename() const { return file_manager_->filename(); }
    
    /**
     * @brief Flush changes to disk
     */
    void flush();

private:
    // Get bucket for a hash value
    ShmBucket* get_bucket(uint32_t hash);
    const ShmBucket* get_bucket(uint32_t hash) const;
    
    // Find element in bucket chain
    ShmNode* find_in_bucket(ShmBucket* bucket, const uint8_t* data, size_t size, 
                           uint32_t hash, ShmNode** prev_out = nullptr);
    
    // Allocate and free nodes
    ShmNode* allocate_node(size_t data_size);
    void free_node(ShmNode* node);

    std::unique_ptr<MMapFileManager> file_manager_;
    HashTableHeader* header_;
    ShmBucket* buckets_;
    CollectionStats stats_;
};

} // namespace fastcollection

#endif // FASTCOLLECTION_SET_H
