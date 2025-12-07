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
 * @file fc_map.h
 * @brief High-performance memory-mapped map implementation with TTL support
 * 
 * ============================================================================
 * FASTCOLLECTION MAP - MEMORY-MAPPED HASH MAP WITH TTL
 * ============================================================================
 * 
 * OVERVIEW:
 * ---------
 * FastMap is an ultra high-performance key-value store that persists data in
 * memory-mapped files. It combines the speed of in-memory hash tables with
 * automatic persistence and TTL-based expiration.
 * 
 * KEY FEATURES:
 * 1. PERSISTENCE: Key-value pairs survive process restarts
 * 2. IPC: Multiple processes can share the same map
 * 3. TTL: Entries auto-expire after configurable duration
 * 4. CACHE-LIKE: Use as a persistent cache with automatic eviction
 * 5. ATOMIC OPS: putIfAbsent, replace, conditional remove
 * 
 * STORAGE ARCHITECTURE:
 * ---------------------
 * +------------------+
 * | HashTableHeader  |  <- Metadata, bucket count, statistics
 * +------------------+
 * | Bucket[0]        |  -> KV -> KV -> KV (separate chaining)
 * +------------------+
 * | Bucket[1]        |  -> KV -> KV
 * +------------------+
 * | ...              |
 * +------------------+
 * 
 * Each key-value pair is stored in a ShmKeyValue structure:
 * +------------------+
 * | ShmEntry header  |  <- hash, TTL, timestamps, state
 * +------------------+
 * | key_size         |
 * | value_size       |
 * +------------------+
 * | key bytes        |
 * | value bytes      |
 * +------------------+
 * 
 * TTL (TIME-TO-LIVE) FEATURE:
 * ---------------------------
 * Each entry can have an individual TTL:
 * 
 * - ttl_seconds = -1 (default): Entry never expires
 * - ttl_seconds = 0: Entry expires immediately
 * - ttl_seconds > 0: Entry expires after specified seconds
 * 
 * TTL behavior:
 * - get() returns empty for expired entries
 * - put() can update TTL for existing keys
 * - Expired entries are lazily cleaned up
 * - containsKey() returns false for expired entries
 * 
 * USE CASES:
 * ----------
 * 
 * 1. DISTRIBUTED CACHE:
 *    // Cache with 5-minute expiry
 *    map.put(cacheKey, cacheValue, 300);
 *    auto value = map.get(cacheKey);
 *    
 * 2. SESSION STORE:
 *    // 30-minute session timeout
 *    map.put(sessionId, userData, 1800);
 *    // Extend session on activity
 *    map.setTTL(sessionId, 1800);
 *    
 * 3. RATE LIMITING:
 *    // Track request counts with 1-minute window
 *    auto count = map.getOrDefault(clientId, 0);
 *    if (count >= MAX_REQUESTS) return TOO_MANY_REQUESTS;
 *    map.put(clientId, count + 1, 60);
 *    
 * 4. FEATURE FLAGS:
 *    // Temporary feature flags with auto-expiry
 *    map.put("beta_feature_x", "enabled", 86400);  // 24-hour test
 * 
 * PERFORMANCE CHARACTERISTICS:
 * ----------------------------
 * Operation        | Average Time | Notes
 * -----------------|--------------|----------------------------------
 * put              | < 400ns      | Includes TTL setup
 * get              | < 150ns      | Lock-free optimistic read
 * remove           | < 350ns      | 
 * containsKey      | < 100ns      | Hash-based lookup
 * putIfAbsent      | < 500ns      | Atomic check-and-set
 * replace          | < 500ns      | Conditional update
 * 
 * Concurrent throughput: > 4M ops/sec with 8 threads
 * 
 * COMPARISON WITH JAVA MAP:
 * -------------------------
 * Feature            | Java HashMap | FastMap
 * -------------------|--------------|------------------
 * Persistence        | No           | Yes (auto)
 * TTL Support        | No           | Yes (per-entry)
 * IPC Sharing        | No           | Yes
 * Process Restart    | Data lost    | Data preserved
 * Memory Mapping     | No           | Yes
 * Atomic Operations  | No*          | Yes
 * 
 * *ConcurrentHashMap has atomic ops but no TTL or persistence
 * 
 * USAGE EXAMPLES:
 * ---------------
 * 
 * C++:
 *   FastMap map("/tmp/mymap.fc", 64*1024*1024, true);
 *   
 *   // Put with TTL
 *   std::string key = "user:123";
 *   std::string value = "{\"name\":\"John\"}";
 *   map.put((uint8_t*)key.data(), key.size(),
 *           (uint8_t*)value.data(), value.size(), 3600);  // 1-hour TTL
 *   
 *   // Get value
 *   std::vector<uint8_t> result;
 *   if (map.get((uint8_t*)key.data(), key.size(), result)) {
 *       // Use result...
 *   }
 *   
 *   // Atomic putIfAbsent
 *   if (map.putIfAbsent(key, value, 3600)) {
 *       // New entry was created
 *   }
 * 
 * Java (via JNI):
 *   FastCollectionMap<String, User> map = new FastCollectionMap<>("/tmp/mymap.fc");
 *   map.put("user:123", user, 3600);  // 1-hour TTL
 *   User u = map.get("user:123");
 * 
 * Python (via pybind11):
 *   m = FastMap("/tmp/mymap.fc")
 *   m.put(key, value, ttl=3600)
 *   value = m.get(key)
 */

#ifndef FASTCOLLECTION_MAP_H
#define FASTCOLLECTION_MAP_H

#include "fc_common.h"
#include "fc_serialization.h"
#include <functional>
#include <vector>
#include <optional>

namespace fastcollection {

/**
 * @brief Ultra high-performance memory-mapped hash map with TTL support
 * 
 * Features:
 * - O(1) average put, get, remove operations
 * - TTL support for automatic entry expiration
 * - Atomic conditional operations (putIfAbsent, replace)
 * - Lock-striped concurrency (per-bucket locking)
 * - Memory-mapped backing for persistence and IPC
 * 
 * Performance targets:
 * - Put: < 400ns average
 * - Get: < 150ns average
 * - Remove: < 350ns average
 * - Concurrent throughput: > 4M ops/sec with 8 threads
 */
class FastMap {
public:
    /**
     * @brief Construct a FastMap with the given memory-mapped file
     * 
     * @param mmap_file Path to the memory-mapped file
     * @param initial_size Initial size of the memory-mapped region
     * @param create_new If true, create a new file (truncating any existing)
     * @param bucket_count Number of hash buckets (must be power of 2)
     * 
     * @throws FastCollectionException if file cannot be created/opened
     */
    FastMap(const std::string& mmap_file,
            size_t initial_size = DEFAULT_INITIAL_SIZE,
            bool create_new = false,
            uint32_t bucket_count = HashTableHeader::DEFAULT_BUCKET_COUNT);
    
    ~FastMap();
    
    // Non-copyable
    FastMap(const FastMap&) = delete;
    FastMap& operator=(const FastMap&) = delete;
    
    // Movable
    FastMap(FastMap&&) noexcept;
    FastMap& operator=(FastMap&&) noexcept;
    
    // =========================================================================
    // PUT OPERATIONS
    // =========================================================================
    
    /**
     * @brief Put a key-value pair into the map
     * 
     * @param key Pointer to key data
     * @param key_size Size of key in bytes
     * @param value Pointer to value data
     * @param value_size Size of value in bytes
     * @param ttl_seconds Time-to-live in seconds (-1 for infinite)
     * @return true if entry was added/updated
     * 
     * If key exists, value and TTL are updated.
     */
    bool put(const uint8_t* key, size_t key_size,
             const uint8_t* value, size_t value_size,
             int32_t ttl_seconds = TTL_INFINITE);
    
    /**
     * @brief Put if key does not exist (atomic)
     * 
     * @param key Pointer to key data
     * @param key_size Size of key in bytes
     * @param value Pointer to value data
     * @param value_size Size of value in bytes
     * @param ttl_seconds Time-to-live in seconds (-1 for infinite)
     * @return true if entry was added, false if key already exists
     * 
     * This is an atomic operation - no race conditions between check and put.
     */
    bool putIfAbsent(const uint8_t* key, size_t key_size,
                     const uint8_t* value, size_t value_size,
                     int32_t ttl_seconds = TTL_INFINITE);
    
    // =========================================================================
    // GET OPERATIONS
    // =========================================================================
    
    /**
     * @brief Get value for a key
     * 
     * @param key Pointer to key data
     * @param key_size Size of key in bytes
     * @param out_value Output buffer for value
     * @return true if key found and not expired
     */
    bool get(const uint8_t* key, size_t key_size,
             std::vector<uint8_t>& out_value) const;
    
    /**
     * @brief Get value or return default
     * 
     * @param key Pointer to key data
     * @param key_size Size of key in bytes
     * @param default_value Default value to return if key not found
     * @return Value if found, default_value otherwise
     */
    std::vector<uint8_t> getOrDefault(const uint8_t* key, size_t key_size,
                                       const std::vector<uint8_t>& default_value) const;
    
    /**
     * @brief Get remaining TTL for a key
     * 
     * @param key Pointer to key data
     * @param key_size Size of key in bytes
     * @return Remaining seconds (-1 if infinite, 0 if expired/not found)
     */
    int64_t getTTL(const uint8_t* key, size_t key_size) const;
    
    // =========================================================================
    // REMOVE OPERATIONS
    // =========================================================================
    
    /**
     * @brief Remove a key from the map
     * 
     * @param key Pointer to key data
     * @param key_size Size of key in bytes
     * @param out_value Optional output buffer for removed value
     * @return true if key was found and removed
     */
    bool remove(const uint8_t* key, size_t key_size,
                std::vector<uint8_t>* out_value = nullptr);
    
    /**
     * @brief Remove only if value matches (atomic)
     * 
     * @param key Pointer to key data
     * @param key_size Size of key in bytes
     * @param expected_value Expected value for removal
     * @param value_size Size of expected value
     * @return true if key was found with matching value and removed
     */
    bool remove(const uint8_t* key, size_t key_size,
                const uint8_t* expected_value, size_t value_size);
    
    /**
     * @brief Remove all expired entries
     * 
     * @return Number of entries removed
     */
    size_t removeExpired();
    
    // =========================================================================
    // REPLACE OPERATIONS
    // =========================================================================
    
    /**
     * @brief Replace value for existing key
     * 
     * @param key Pointer to key data
     * @param key_size Size of key in bytes
     * @param value New value data
     * @param value_size Size of new value
     * @param ttl_seconds New TTL (-1 for infinite)
     * @return true if key existed and was replaced
     */
    bool replace(const uint8_t* key, size_t key_size,
                 const uint8_t* value, size_t value_size,
                 int32_t ttl_seconds = TTL_INFINITE);
    
    /**
     * @brief Replace only if current value matches (atomic)
     * 
     * @param key Pointer to key data
     * @param key_size Size of key in bytes
     * @param old_value Expected current value
     * @param old_value_size Size of expected value
     * @param new_value New value to set
     * @param new_value_size Size of new value
     * @param ttl_seconds New TTL (-1 for infinite)
     * @return true if value was replaced
     */
    bool replace(const uint8_t* key, size_t key_size,
                 const uint8_t* old_value, size_t old_value_size,
                 const uint8_t* new_value, size_t new_value_size,
                 int32_t ttl_seconds = TTL_INFINITE);
    
    /**
     * @brief Update TTL for existing key without changing value
     * 
     * @param key Pointer to key data
     * @param key_size Size of key in bytes
     * @param ttl_seconds New TTL in seconds (-1 for infinite)
     * @return true if key existed and TTL was updated
     */
    bool setTTL(const uint8_t* key, size_t key_size, int32_t ttl_seconds);
    
    // =========================================================================
    // QUERY OPERATIONS
    // =========================================================================
    
    /**
     * @brief Check if key exists (and is not expired)
     * 
     * @param key Pointer to key data
     * @param key_size Size of key in bytes
     * @return true if key exists and is not expired
     */
    bool containsKey(const uint8_t* key, size_t key_size) const;
    
    /**
     * @brief Check if value exists in any entry
     * 
     * @param value Pointer to value data
     * @param value_size Size of value in bytes
     * @return true if value exists in map
     * 
     * Time Complexity: O(n) - requires full scan
     */
    bool containsValue(const uint8_t* value, size_t value_size) const;
    
    // =========================================================================
    // ITERATION
    // =========================================================================
    
    /**
     * @brief Iterate over all non-expired key-value pairs
     * 
     * @param callback Function receiving key data, key size, value data, value size
     */
    void forEach(std::function<bool(const uint8_t* key, size_t key_size,
                                    const uint8_t* value, size_t value_size)> callback) const;
    
    /**
     * @brief Iterate with TTL information
     */
    void forEachWithTTL(std::function<bool(const uint8_t* key, size_t key_size,
                                           const uint8_t* value, size_t value_size,
                                           int64_t ttl_remaining)> callback) const;
    
    /**
     * @brief Iterate over keys only
     */
    void forEachKey(std::function<bool(const uint8_t* key, size_t key_size)> callback) const;
    
    /**
     * @brief Iterate over values only
     */
    void forEachValue(std::function<bool(const uint8_t* value, size_t value_size)> callback) const;
    
    /**
     * @brief Get all keys as a set
     */
    std::vector<std::vector<uint8_t>> keySet() const;
    
    /**
     * @brief Get all values as a collection
     */
    std::vector<std::vector<uint8_t>> values() const;
    
    // =========================================================================
    // UTILITY
    // =========================================================================
    
    /**
     * @brief Clear all entries from the map
     */
    void clear();
    
    /**
     * @brief Get the number of non-expired entries
     */
    size_t size() const;
    
    /**
     * @brief Check if map is empty
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
    // Get bucket for a key hash
    ShmBucket* get_bucket(uint32_t hash);
    const ShmBucket* get_bucket(uint32_t hash) const;
    
    // Find key-value in bucket chain
    ShmKeyValue* find_in_bucket(ShmBucket* bucket, const uint8_t* key, size_t key_size,
                                uint32_t hash, ShmKeyValue** prev_out = nullptr);
    
    // Allocate and free key-value nodes
    ShmKeyValue* allocate_kv(size_t key_size, size_t value_size);
    void free_kv(ShmKeyValue* kv);

    std::unique_ptr<MMapFileManager> file_manager_;
    HashTableHeader* header_;
    ShmBucket* buckets_;
    CollectionStats stats_;
};

} // namespace fastcollection

#endif // FASTCOLLECTION_MAP_H
