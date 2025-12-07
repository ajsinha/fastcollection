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
 * @file fc_serialization.h
 * @brief Serialization utilities for storing Java objects in shared memory
 */

#ifndef FASTCOLLECTION_SERIALIZATION_H
#define FASTCOLLECTION_SERIALIZATION_H

#include "fc_common.h"
#include <vector>
#include <cstring>

namespace fastcollection {

/**
 * @brief Entry stored in shared memory representing a serialized object
 * 
 * This is the fundamental unit of storage in FastCollection. Each entry contains:
 * - A header with metadata (size, hash, timestamp)
 * - The serialized byte array from Java
 * 
 * The structure is designed for:
 * - Cache-line alignment (64 bytes) to prevent false sharing
 * - Lock-free reads via atomic operations
 * - Efficient memory layout for SIMD operations
 */
/**
 * @brief Entry stored in memory-mapped file representing a serialized object
 * 
 * This is the fundamental unit of storage in FastCollection. Each entry contains:
 * - A header with metadata (size, hash, timestamp, TTL)
 * - The serialized byte array from Java/Python
 * 
 * The structure is designed for:
 * - Cache-line alignment (64 bytes) to prevent false sharing
 * - Lock-free reads via atomic operations
 * - Efficient memory layout for SIMD operations
 * - TTL (Time-To-Live) support for automatic expiration
 * 
 * TTL Behavior:
 * - ttl_seconds = -1 (TTL_INFINITE): Entry never expires
 * - ttl_seconds = 0: Entry expires immediately (useful for testing)
 * - ttl_seconds > 0: Entry expires after specified seconds from creation
 */
struct alignas(64) ShmEntry {
    std::atomic<uint32_t> state;     // 0=empty, 1=writing, 2=valid, 3=deleted, 4=expired
    uint32_t data_size;              // Size of serialized data
    uint32_t hash_code;              // Hash for quick equality check
    int32_t ttl_seconds;             // TTL in seconds (-1 = infinite, no expiry)
    uint64_t created_at;             // Creation timestamp in nanoseconds
    uint64_t expires_at;             // Expiration timestamp in nanoseconds (0 = never)
    uint64_t version;                // Version number for optimistic locking
    
    // States
    static constexpr uint32_t STATE_EMPTY = 0;
    static constexpr uint32_t STATE_WRITING = 1;
    static constexpr uint32_t STATE_VALID = 2;
    static constexpr uint32_t STATE_DELETED = 3;
    static constexpr uint32_t STATE_EXPIRED = 4;
    
    ShmEntry() : state(STATE_EMPTY), data_size(0), hash_code(0), 
                 ttl_seconds(TTL_INFINITE), created_at(0), expires_at(0), version(0) {}
    
    bool try_acquire_for_write() {
        uint32_t expected = STATE_EMPTY;
        return state.compare_exchange_strong(expected, STATE_WRITING,
            std::memory_order_acq_rel, std::memory_order_relaxed);
    }
    
    void mark_valid() {
        state.store(STATE_VALID, std::memory_order_release);
    }
    
    void mark_deleted() {
        state.store(STATE_DELETED, std::memory_order_release);
    }
    
    void mark_expired() {
        state.store(STATE_EXPIRED, std::memory_order_release);
    }
    
    bool is_valid() const {
        return state.load(std::memory_order_acquire) == STATE_VALID;
    }
    
    bool is_expired() const {
        uint32_t s = state.load(std::memory_order_acquire);
        if (s == STATE_EXPIRED) return true;
        if (s != STATE_VALID) return false;
        if (expires_at == 0) return false;  // No expiration set
        return current_timestamp_ns() >= expires_at;
    }
    
    /**
     * @brief Check if entry is valid and not expired
     * @return true if entry can be read
     */
    bool is_alive() const {
        if (!is_valid()) return false;
        if (expires_at == 0) return true;  // No expiration
        return current_timestamp_ns() < expires_at;
    }
    
    /**
     * @brief Set TTL for this entry
     * @param ttl TTL in seconds (-1 for infinite)
     */
    void set_ttl(int32_t ttl) {
        ttl_seconds = ttl;
        created_at = current_timestamp_ns();
        if (ttl < 0) {
            expires_at = 0;  // Never expires
        } else {
            expires_at = created_at + (static_cast<uint64_t>(ttl) * 1000000000ULL);
        }
    }
    
    /**
     * @brief Get remaining TTL in seconds
     * @return Remaining seconds, 0 if expired, -1 if infinite
     */
    int64_t remaining_ttl_seconds() const {
        if (ttl_seconds < 0) return -1;
        if (expires_at == 0) return -1;
        uint64_t now = current_timestamp_ns();
        if (now >= expires_at) return 0;
        return static_cast<int64_t>((expires_at - now) / 1000000000ULL);
    }
};

/**
 * @brief Node for linked structures (list, queue, stack) in shared memory
 */
struct ShmNode {
    ShmEntry entry;
    std::atomic<int64_t> next_offset;  // Offset to next node (-1 if none)
    std::atomic<int64_t> prev_offset;  // Offset to prev node (-1 if none)
    uint8_t data[0];                    // Flexible array for serialized data
    
    static constexpr int64_t NULL_OFFSET = -1;
    
    ShmNode() : next_offset(NULL_OFFSET), prev_offset(NULL_OFFSET) {}
    
    static size_t total_size(size_t data_size) {
        // Align to 64 bytes for cache efficiency
        size_t base = sizeof(ShmNode) + data_size;
        return (base + 63) & ~63;
    }
};

/**
 * @brief Key-value pair for map storage
 */
struct ShmKeyValue {
    ShmEntry entry;
    std::atomic<int64_t> next_offset;  // Offset to next key-value in bucket chain (-1 if none)
    std::atomic<int64_t> prev_offset;  // Offset to prev key-value in bucket chain (-1 if none)
    uint32_t key_size;
    uint32_t value_size;
    uint8_t data[0];  // key bytes followed by value bytes
    
    static constexpr int64_t NULL_OFFSET = -1;
    
    ShmKeyValue() : next_offset(NULL_OFFSET), prev_offset(NULL_OFFSET), key_size(0), value_size(0) {}
    
    static size_t total_size(size_t key_size, size_t value_size) {
        size_t base = sizeof(ShmKeyValue) + key_size + value_size;
        return (base + 63) & ~63;
    }
    
    uint8_t* key_data() { return data; }
    const uint8_t* key_data() const { return data; }
    
    uint8_t* value_data() { return data + key_size; }
    const uint8_t* value_data() const { return data + key_size; }
};

/**
 * @brief Bucket for hash-based collections (set, map)
 */
struct ShmBucket {
    IpcMutex mutex;                    // Use regular mutex for exclusive locking
    std::atomic<int64_t> head_offset;  // Offset to first entry in bucket
    std::atomic<uint32_t> count;       // Number of entries in bucket
    std::atomic<uint32_t> size;        // Alias for count (for compatibility)
    
    static constexpr int64_t NULL_OFFSET = -1;
    
    ShmBucket() : head_offset(NULL_OFFSET), count(0), size(0) {}
};

/**
 * @brief Header structure stored at the beginning of each collection's segment
 */
struct CollectionHeader {
    uint32_t magic;              // Magic number for validation
    uint32_t version;            // Data structure version
    uint64_t created_at;         // Creation timestamp
    uint64_t modified_at;        // Last modification timestamp
    std::atomic<uint64_t> size;  // Number of elements
    std::atomic<uint64_t> capacity;
    IpcSharedMutex global_mutex; // Global mutex for structural changes
    
    static constexpr uint32_t MAGIC = 0xFAC01EC0;
    static constexpr uint32_t CURRENT_VERSION = 1;
    
    CollectionHeader() 
        : magic(MAGIC)
        , version(CURRENT_VERSION)
        , created_at(current_timestamp_ns())
        , modified_at(created_at)
        , size(0)
        , capacity(0) {}
    
    bool is_valid() const {
        return magic == MAGIC && version == CURRENT_VERSION;
    }
};

/**
 * @brief List-specific header
 */
struct ListHeader : public CollectionHeader {
    std::atomic<int64_t> head_offset;
    std::atomic<int64_t> tail_offset;
    
    ListHeader() : head_offset(ShmNode::NULL_OFFSET), tail_offset(ShmNode::NULL_OFFSET) {}
};

/**
 * @brief Hash table header for set and map
 */
struct HashTableHeader : public CollectionHeader {
    uint32_t bucket_count;
    uint32_t load_factor_percent;  // Default 75%
    std::atomic<uint64_t> total_bytes;
    
    static constexpr uint32_t DEFAULT_BUCKET_COUNT = 16384;
    static constexpr uint32_t DEFAULT_LOAD_FACTOR = 75;
    
    HashTableHeader() 
        : bucket_count(DEFAULT_BUCKET_COUNT)
        , load_factor_percent(DEFAULT_LOAD_FACTOR)
        , total_bytes(0) {}
    
    explicit HashTableHeader(uint32_t buckets)
        : bucket_count(buckets > 0 ? buckets : DEFAULT_BUCKET_COUNT)
        , load_factor_percent(DEFAULT_LOAD_FACTOR)
        , total_bytes(0) {}
};

/**
 * @brief Queue/Stack header with specialized pointers
 */
struct DequeHeader : public CollectionHeader {
    std::atomic<int64_t> front_offset;
    std::atomic<int64_t> back_offset;
    
    DequeHeader() : front_offset(ShmNode::NULL_OFFSET), back_offset(ShmNode::NULL_OFFSET) {}
};

/**
 * @brief Utility class for serialization operations
 */
class SerializationUtil {
public:
    /**
     * @brief Copy data into a shared memory node with TTL support
     * @param node Target node
     * @param data Source data
     * @param size Data size
     * @param ttl_seconds TTL in seconds (-1 for infinite, no expiry)
     */
    static void copy_to_node(ShmNode* node, const uint8_t* data, size_t size, 
                            int32_t ttl_seconds = TTL_INFINITE) {
        node->entry.data_size = static_cast<uint32_t>(size);
        node->entry.hash_code = compute_hash(data, size);
        node->entry.set_ttl(ttl_seconds);
        std::memcpy(node->data, data, size);
        node->entry.mark_valid();
    }
    
    /**
     * @brief Copy data from a shared memory node
     * @param node Source node
     * @return Vector containing the data (empty if expired)
     */
    static std::vector<uint8_t> copy_from_node(const ShmNode* node) {
        if (!node->entry.is_alive()) {
            return std::vector<uint8_t>();  // Return empty for expired entries
        }
        std::vector<uint8_t> result(node->entry.data_size);
        std::memcpy(result.data(), node->data, node->entry.data_size);
        return result;
    }
    
    /**
     * @brief Copy key-value data into a shared memory structure with TTL
     * @param kv Target key-value structure
     * @param key Key data
     * @param key_size Key size
     * @param value Value data
     * @param value_size Value size
     * @param ttl_seconds TTL in seconds (-1 for infinite)
     */
    static void copy_to_kv(ShmKeyValue* kv, 
                          const uint8_t* key, size_t key_size,
                          const uint8_t* value, size_t value_size,
                          int32_t ttl_seconds = TTL_INFINITE) {
        kv->key_size = static_cast<uint32_t>(key_size);
        kv->value_size = static_cast<uint32_t>(value_size);
        kv->entry.data_size = static_cast<uint32_t>(key_size + value_size);
        kv->entry.hash_code = compute_hash(key, key_size);
        kv->entry.set_ttl(ttl_seconds);
        std::memcpy(kv->data, key, key_size);
        std::memcpy(kv->data + key_size, value, value_size);
        kv->entry.mark_valid();
    }
    
    /**
     * @brief Compare two byte arrays
     */
    static bool bytes_equal(const uint8_t* a, size_t a_size,
                           const uint8_t* b, size_t b_size) {
        if (a_size != b_size) return false;
        return std::memcmp(a, b, a_size) == 0;
    }
    
    /**
     * @brief Compute bucket index for a given hash
     */
    static uint32_t bucket_index(uint32_t hash, uint32_t bucket_count) {
        return hash & (bucket_count - 1);  // Assumes power-of-2 bucket count
    }
};

} // namespace fastcollection

#endif // FASTCOLLECTION_SERIALIZATION_H
