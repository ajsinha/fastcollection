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
 * @file fc_common.h
 * @brief Common types, utilities, and memory management for FastCollection
 */

#ifndef FASTCOLLECTION_COMMON_H
#define FASTCOLLECTION_COMMON_H

#include <cstdint>
#include <cstddef>
#include <string>
#include <memory>
#include <atomic>
#include <functional>
#include <stdexcept>
#include <chrono>

#include <boost/interprocess/managed_mapped_file.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/containers/map.hpp>
#include <boost/interprocess/containers/set.hpp>
#include <boost/interprocess/containers/deque.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/interprocess_sharable_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/sync/sharable_lock.hpp>

namespace fastcollection {

// Version information
constexpr uint32_t FC_VERSION_MAJOR = 1;
constexpr uint32_t FC_VERSION_MINOR = 0;
constexpr uint32_t FC_VERSION_PATCH = 0;

#define FASTCOLLECTION_VERSION_STRING "1.0.0"

// Default configuration
constexpr size_t DEFAULT_INITIAL_SIZE = 64 * 1024 * 1024;  // 64MB
constexpr size_t DEFAULT_GROWTH_SIZE = 16 * 1024 * 1024;   // 16MB
constexpr size_t MAX_SERIALIZED_SIZE = 16 * 1024 * 1024;   // 16MB per object

// TTL (Time-To-Live) configuration
constexpr int64_t TTL_INFINITE = -1;                       // No expiration
constexpr int64_t TTL_DEFAULT = TTL_INFINITE;              // Default: no expiration
constexpr uint64_t TTL_CLEANUP_INTERVAL_MS = 1000;         // Background cleanup interval
constexpr size_t TTL_CLEANUP_BATCH_SIZE = 100;             // Max items to cleanup per pass

namespace bip = boost::interprocess;

// Forward declarations
class FastCollectionException;
class SerializedObject;
template<typename T> class MMapAllocator;

/**
 * @brief Custom exception for FastCollection operations
 */
class FastCollectionException : public std::runtime_error {
public:
    enum class ErrorCode {
        SUCCESS = 0,
        MEMORY_ALLOCATION_FAILED,
        FILE_CREATION_FAILED,
        FILE_OPEN_FAILED,
        SERIALIZATION_FAILED,
        DESERIALIZATION_FAILED,
        INDEX_OUT_OF_BOUNDS,
        KEY_NOT_FOUND,
        NOT_FOUND,
        COLLECTION_FULL,
        LOCK_TIMEOUT,
        INVALID_ARGUMENT,
        INTERNAL_ERROR,
        TIMEOUT,
        ELEMENT_EXPIRED
    };

    explicit FastCollectionException(ErrorCode code, const std::string& message)
        : std::runtime_error(message), error_code_(code) {}

    ErrorCode code() const noexcept { return error_code_; }

private:
    ErrorCode error_code_;
};

/**
 * @brief Represents a serialized Java object stored in shared memory
 * 
 * This structure is designed for cache-line alignment (64 bytes) to minimize
 * false sharing in concurrent scenarios.
 */
struct alignas(64) SerializedObject {
    uint32_t size;           // Size of serialized data
    uint32_t hash;           // Hash code for quick comparison
    uint64_t timestamp;      // Creation/modification timestamp
    uint8_t data[0];         // Flexible array member for actual data

    static size_t total_size(size_t data_size) {
        return sizeof(SerializedObject) + data_size;
    }
};

/**
 * @brief Memory-mapped file segment manager type
 */
using SegmentManager = bip::managed_mapped_file::segment_manager;

/**
 * @brief Allocator for bytes in shared memory
 */
using ByteAllocator = bip::allocator<uint8_t, SegmentManager>;

/**
 * @brief Shared memory string type
 */
using ShmString = bip::basic_string<char, std::char_traits<char>,
    bip::allocator<char, SegmentManager>>;

/**
 * @brief Shared memory byte vector type
 */
using ShmByteVector = bip::vector<uint8_t, ByteAllocator>;

/**
 * @brief Interprocess mutex for exclusive access
 */
using IpcMutex = bip::interprocess_mutex;

/**
 * @brief Interprocess shared mutex for reader-writer locks
 */
using IpcSharedMutex = bip::interprocess_sharable_mutex;

/**
 * @brief Scoped exclusive lock
 */
using ScopedLock = bip::scoped_lock<IpcMutex>;

/**
 * @brief Scoped shared lock for readers
 */
using ScopedSharedLock = bip::sharable_lock<IpcSharedMutex>;

/**
 * @brief Statistics for a collection
 */
struct CollectionStats {
    std::atomic<uint64_t> size{0};
    std::atomic<uint64_t> capacity{0};
    std::atomic<uint64_t> total_bytes{0};
    std::atomic<uint64_t> read_count{0};
    std::atomic<uint64_t> write_count{0};
    std::atomic<uint64_t> hit_count{0};
    std::atomic<uint64_t> miss_count{0};
    
    void reset() {
        size.store(0, std::memory_order_relaxed);
        capacity.store(0, std::memory_order_relaxed);
        total_bytes.store(0, std::memory_order_relaxed);
        read_count.store(0, std::memory_order_relaxed);
        write_count.store(0, std::memory_order_relaxed);
        hit_count.store(0, std::memory_order_relaxed);
        miss_count.store(0, std::memory_order_relaxed);
    }
};

/**
 * @brief Configuration options for collections
 */
struct CollectionConfig {
    size_t initial_size = DEFAULT_INITIAL_SIZE;
    size_t growth_size = DEFAULT_GROWTH_SIZE;
    bool auto_grow = true;
    bool enable_stats = true;
    uint32_t lock_timeout_ms = 5000;
};

/**
 * @brief High-resolution timer for performance measurement
 */
class PerfTimer {
public:
    using Clock = std::chrono::high_resolution_clock;
    using TimePoint = Clock::time_point;
    using Nanoseconds = std::chrono::nanoseconds;

    void start() { start_ = Clock::now(); }
    
    void stop() { end_ = Clock::now(); }
    
    int64_t elapsed_ns() const {
        return std::chrono::duration_cast<Nanoseconds>(end_ - start_).count();
    }
    
    double elapsed_us() const {
        return elapsed_ns() / 1000.0;
    }
    
    double elapsed_ms() const {
        return elapsed_ns() / 1000000.0;
    }

private:
    TimePoint start_;
    TimePoint end_;
};

/**
 * @brief RAII wrapper for memory-mapped file management
 */
class MMapFileManager {
public:
    explicit MMapFileManager(const std::string& filename, 
                            size_t initial_size = DEFAULT_INITIAL_SIZE,
                            bool create_new = false);
    
    ~MMapFileManager();
    
    // Non-copyable
    MMapFileManager(const MMapFileManager&) = delete;
    MMapFileManager& operator=(const MMapFileManager&) = delete;
    
    // Movable
    MMapFileManager(MMapFileManager&&) noexcept;
    MMapFileManager& operator=(MMapFileManager&&) noexcept;
    
    /**
     * @brief Get the segment manager for allocations
     */
    SegmentManager* segment_manager();
    
    /**
     * @brief Get or create a named object in shared memory
     */
    template<typename T, typename... Args>
    T* find_or_construct(const char* name, Args&&... args) {
        return file_->find_or_construct<T>(name)(std::forward<Args>(args)...);
    }
    
    /**
     * @brief Find an existing named object
     */
    template<typename T>
    std::pair<T*, size_t> find(const char* name) {
        return file_->find<T>(name);
    }
    
    /**
     * @brief Construct an array of objects in shared memory
     */
    template<typename T>
    T* construct_array(const char* name, size_t count) {
        return file_->find_or_construct<T>(name)[count]();
    }
    
    /**
     * @brief Destroy a named object
     */
    template<typename T>
    void destroy(const char* name) {
        file_->destroy<T>(name);
    }
    
    /**
     * @brief Allocate raw bytes
     */
    void* allocate(size_t bytes);
    
    /**
     * @brief Deallocate raw bytes
     */
    void deallocate(void* ptr);
    
    /**
     * @brief Grow the mapped file if needed
     */
    bool grow(size_t additional_bytes);
    
    /**
     * @brief Get free space in the mapped file
     */
    size_t free_space() const;
    
    /**
     * @brief Get total size of the mapped file
     */
    size_t size() const;
    
    /**
     * @brief Flush changes to disk
     */
    void flush();
    
    /**
     * @brief Get the filename
     */
    const std::string& filename() const { return filename_; }

private:
    std::string filename_;
    std::unique_ptr<bip::managed_mapped_file> file_;
    size_t growth_size_;
};

/**
 * @brief Compute hash code for serialized data
 */
inline uint32_t compute_hash(const uint8_t* data, size_t size) {
    // FNV-1a hash - fast and good distribution
    uint32_t hash = 2166136261u;
    for (size_t i = 0; i < size; ++i) {
        hash ^= data[i];
        hash *= 16777619u;
    }
    return hash;
}

/**
 * @brief Get current timestamp in nanoseconds
 */
inline uint64_t current_timestamp_ns() {
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()
        ).count()
    );
}

/**
 * @brief Memory barrier for cache coherency
 */
inline void memory_barrier() {
    std::atomic_thread_fence(std::memory_order_seq_cst);
}

/**
 * @brief Prefetch data into CPU cache
 */
inline void prefetch_read(const void* addr) {
#if defined(__GNUC__) || defined(__clang__)
    __builtin_prefetch(addr, 0, 3);
#endif
}

inline void prefetch_write(void* addr) {
#if defined(__GNUC__) || defined(__clang__)
    __builtin_prefetch(addr, 1, 3);
#endif
}

/**
 * @brief Statistics about a collection file
 */
struct FileStats {
    size_t total_size;      // Total file size
    size_t free_size;       // Free space in file
    size_t used_size;       // Used space in file
    uint32_t element_count; // Number of elements
    uint64_t created_at;    // Creation timestamp
    uint64_t modified_at;   // Last modification timestamp
};

// Library initialization functions
void initialize();
void shutdown();
const char* version();
bool deleteCollectionFile(const std::string& filename);
bool isValidCollectionFile(const std::string& filename);
bool getFileStats(const std::string& filename, FileStats& stats);

} // namespace fastcollection

#endif // FASTCOLLECTION_COMMON_H
