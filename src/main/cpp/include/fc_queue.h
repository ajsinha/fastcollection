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
 * @file fc_queue.h
 * @brief High-performance memory-mapped queue implementation with TTL support
 * 
 * ============================================================================
 * FASTCOLLECTION QUEUE - MEMORY-MAPPED DEQUE WITH TTL
 * ============================================================================
 * 
 * OVERVIEW:
 * ---------
 * FastQueue is an ultra high-performance double-ended queue (deque) that stores
 * data in memory-mapped files. It supports both FIFO queue and deque operations
 * with automatic element expiration via TTL.
 * 
 * KEY FEATURES:
 * 1. PERSISTENCE: Queue survives process restarts
 * 2. IPC: Multiple processes can share the same queue (producer/consumer)
 * 3. TTL: Elements auto-expire after configurable duration
 * 4. BLOCKING OPS: poll/take with timeout support
 * 5. DEQUE: Support for both ends (offerFirst, offerLast, pollFirst, pollLast)
 * 
 * QUEUE ARCHITECTURE:
 * -------------------
 * The queue is implemented as a doubly-linked list with head and tail pointers:
 * 
 * +------------------+
 * | DequeHeader      |  <- front_offset, back_offset, size
 * +------------------+
 *        |
 *        v
 * [Front] <-> [Node] <-> [Node] <-> [Node] <-> [Back]
 *   ^                                            ^
 *   |                                            |
 * pollFirst()                               offerLast()
 * 
 * TTL (TIME-TO-LIVE) FEATURE:
 * ---------------------------
 * Elements can expire automatically:
 * 
 * - ttl_seconds = -1 (default): Element never expires
 * - ttl_seconds = 0: Element expires immediately
 * - ttl_seconds > 0: Element expires after specified seconds
 * 
 * TTL behavior in queues:
 * - Expired elements are skipped during poll/take
 * - peek() returns null for expired head elements
 * - Expired elements are lazily cleaned up
 * - drainTo() skips expired elements
 * 
 * USE CASES:
 * ----------
 * 
 * 1. TASK QUEUE WITH TIMEOUT:
 *    // Tasks expire if not processed within 5 minutes
 *    queue.offer(task, 300);
 *    
 *    // Worker
 *    while (true) {
 *        auto task = queue.poll();  // Skips expired tasks
 *        if (task) processTask(task);
 *    }
 *    
 * 2. MESSAGE BUFFER:
 *    // Messages older than 1 hour are dropped
 *    queue.offer(message, 3600);
 *    
 * 3. EVENT STREAM:
 *    // Events have 10-second relevance window
 *    queue.offer(event, 10);
 *    
 * 4. PRIORITY DEQUE:
 *    // High priority at front, low at back
 *    if (highPriority) {
 *        queue.offerFirst(task, 60);  // Front, 1-min TTL
 *    } else {
 *        queue.offerLast(task, 300);  // Back, 5-min TTL
 *    }
 * 
 * PERFORMANCE CHARACTERISTICS:
 * ----------------------------
 * Operation     | Average Time | Notes
 * --------------|--------------|----------------------------------
 * offer         | < 200ns      | O(1) - tail append
 * poll          | < 200ns      | O(1) - head remove
 * peek          | < 50ns       | O(1) - read only
 * offerFirst    | < 200ns      | O(1) - head insert
 * pollLast      | < 200ns      | O(1) - tail remove
 * size          | < 50ns       | O(1) - atomic counter
 * 
 * Concurrent throughput: > 8M ops/sec with 8 threads (4 producer, 4 consumer)
 * 
 * BLOCKING OPERATIONS:
 * --------------------
 * The queue supports blocking operations for producer-consumer patterns:
 * 
 * - take(): Block until element available
 * - poll(timeout): Wait up to timeout for element
 * - put(): Block until space available (unbounded queue - always succeeds)
 * - offer(timeout): Wait up to timeout for space
 * 
 * Note: This is an unbounded queue, so put/offer with timeout behave like
 * regular offer (always succeed immediately).
 * 
 * USAGE EXAMPLES:
 * ---------------
 * 
 * C++:
 *   FastQueue queue("/tmp/myqueue.fc", 64*1024*1024, true);
 *   
 *   // Producer with TTL
 *   std::string msg = "task data";
 *   queue.offer((uint8_t*)msg.data(), msg.size(), 300);  // 5-min TTL
 *   
 *   // Consumer
 *   std::vector<uint8_t> task;
 *   if (queue.poll(task)) {
 *       // Process task...
 *   }
 *   
 *   // Blocking consumer
 *   auto data = queue.take();  // Blocks until element available
 * 
 * Java (via JNI):
 *   FastCollectionQueue queue = new FastCollectionQueue("/tmp/myqueue.fc");
 *   queue.offer(task, 300);  // 5-minute TTL
 *   Task t = queue.poll();
 *   
 * Python (via pybind11):
 *   q = FastQueue("/tmp/myqueue.fc")
 *   q.offer(data, ttl=300)
 *   item = q.poll()
 */

#ifndef FASTCOLLECTION_QUEUE_H
#define FASTCOLLECTION_QUEUE_H

#include "fc_common.h"
#include "fc_serialization.h"
#include <functional>
#include <chrono>
#include <vector>

namespace fastcollection {

/**
 * @brief Ultra high-performance memory-mapped concurrent queue with TTL
 * 
 * Features:
 * - O(1) enqueue and dequeue operations
 * - TTL support for automatic element expiration
 * - Blocking operations with timeout support
 * - Deque operations (offerFirst, pollLast, etc.)
 * - Memory-mapped backing for persistence and IPC
 * - FIFO ordering guarantee (excluding expired elements)
 * 
 * Performance targets:
 * - Offer: < 200ns average
 * - Poll: < 200ns average  
 * - Peek: < 50ns average
 * - Concurrent throughput: > 8M ops/sec with 8 threads
 */
class FastQueue {
public:
    /**
     * @brief Construct a FastQueue with the given memory-mapped file
     * 
     * @param mmap_file Path to the memory-mapped file
     * @param initial_size Initial size of the memory-mapped region
     * @param create_new If true, create a new file (truncating any existing)
     * 
     * @throws FastCollectionException if file cannot be created/opened
     */
    FastQueue(const std::string& mmap_file,
              size_t initial_size = DEFAULT_INITIAL_SIZE,
              bool create_new = false);
    
    ~FastQueue();
    
    // Non-copyable
    FastQueue(const FastQueue&) = delete;
    FastQueue& operator=(const FastQueue&) = delete;
    
    // Movable
    FastQueue(FastQueue&&) noexcept;
    FastQueue& operator=(FastQueue&&) noexcept;
    
    // =========================================================================
    // QUEUE INTERFACE (FIFO)
    // =========================================================================
    
    /**
     * @brief Add element to the tail of the queue
     * 
     * @param data Serialized object data
     * @param size Size of the data
     * @param ttl_seconds Time-to-live in seconds (-1 for infinite)
     * @return true if successful
     * 
     * Time Complexity: O(1)
     */
    bool offer(const uint8_t* data, size_t size, int32_t ttl_seconds = TTL_INFINITE);
    
    /**
     * @brief Add element to the tail (alias for offer)
     */
    bool add(const uint8_t* data, size_t size, int32_t ttl_seconds = TTL_INFINITE) { 
        return offer(data, size, ttl_seconds); 
    }
    
    /**
     * @brief Remove and return element from the head
     * 
     * @param out_data Output buffer for the data
     * @return true if element was retrieved (false if queue empty or all expired)
     * 
     * Skips expired elements automatically.
     */
    bool poll(std::vector<uint8_t>& out_data);
    
    /**
     * @brief Remove and return element from the head (throws if empty)
     * 
     * @throws FastCollectionException if queue is empty
     */
    std::vector<uint8_t> remove();
    
    /**
     * @brief Get element at head without removing
     * 
     * @param out_data Output buffer for the data
     * @return true if element exists and not expired
     */
    bool peek(std::vector<uint8_t>& out_data) const;
    
    /**
     * @brief Get element at head (throws if empty)
     * 
     * @throws FastCollectionException if queue is empty
     */
    std::vector<uint8_t> element() const;
    
    // =========================================================================
    // BLOCKING OPERATIONS
    // =========================================================================
    
    /**
     * @brief Put element, blocking if necessary
     * 
     * For unbounded queue, this always succeeds immediately.
     * 
     * @param data Serialized object data
     * @param size Size of the data
     * @param ttl_seconds Time-to-live in seconds (-1 for infinite)
     */
    void put(const uint8_t* data, size_t size, int32_t ttl_seconds = TTL_INFINITE);
    
    /**
     * @brief Offer with timeout
     * 
     * @param data Serialized object data
     * @param size Size of the data
     * @param timeout_ms Timeout in milliseconds
     * @param ttl_seconds Time-to-live in seconds (-1 for infinite)
     * @return true if element was added within timeout
     */
    bool offer(const uint8_t* data, size_t size, uint32_t timeout_ms, 
               int32_t ttl_seconds = TTL_INFINITE);
    
    /**
     * @brief Take element, blocking until one is available
     * 
     * Skips expired elements while waiting.
     * 
     * @return Element data
     */
    std::vector<uint8_t> take();
    
    /**
     * @brief Poll with timeout
     * 
     * @param out_data Output buffer for the data
     * @param timeout_ms Timeout in milliseconds
     * @return true if element was retrieved within timeout
     */
    bool poll(std::vector<uint8_t>& out_data, uint32_t timeout_ms);
    
    // =========================================================================
    // DEQUE OPERATIONS
    // =========================================================================
    
    /**
     * @brief Add element to the front of the queue
     * 
     * @param data Serialized object data
     * @param size Size of the data
     * @param ttl_seconds Time-to-live in seconds (-1 for infinite)
     * @return true if successful
     */
    bool offerFirst(const uint8_t* data, size_t size, int32_t ttl_seconds = TTL_INFINITE);
    
    /**
     * @brief Add element to the back of the queue
     */
    bool offerLast(const uint8_t* data, size_t size, int32_t ttl_seconds = TTL_INFINITE) { 
        return offer(data, size, ttl_seconds); 
    }
    
    /**
     * @brief Remove and return element from front
     */
    bool pollFirst(std::vector<uint8_t>& out_data) { return poll(out_data); }
    
    /**
     * @brief Remove and return element from back
     */
    bool pollLast(std::vector<uint8_t>& out_data);
    
    /**
     * @brief Peek at front element
     */
    bool peekFirst(std::vector<uint8_t>& out_data) const { return peek(out_data); }
    
    /**
     * @brief Peek at back element
     */
    bool peekLast(std::vector<uint8_t>& out_data) const;
    
    // =========================================================================
    // TTL OPERATIONS
    // =========================================================================
    
    /**
     * @brief Get TTL of head element
     * 
     * @return Remaining TTL in seconds, -1 if infinite, 0 if expired/empty
     */
    int64_t peekTTL() const;
    
    /**
     * @brief Remove all expired elements
     * 
     * @return Number of elements removed
     */
    size_t removeExpired();
    
    // =========================================================================
    // UTILITY OPERATIONS
    // =========================================================================
    
    /**
     * @brief Check if queue contains element (not expired)
     * 
     * @param data Data to search for
     * @param size Size of the data
     * @return true if found and not expired
     * 
     * Time Complexity: O(n)
     */
    bool contains(const uint8_t* data, size_t size) const;
    
    /**
     * @brief Remove specific element (first occurrence)
     * 
     * @param data Data to remove
     * @param size Size of the data
     * @return true if found and removed
     */
    bool removeElement(const uint8_t* data, size_t size);
    
    /**
     * @brief Clear all elements from the queue
     */
    void clear();
    
    /**
     * @brief Get the number of non-expired elements
     */
    size_t size() const;
    
    /**
     * @brief Check if queue is empty (no non-expired elements)
     */
    bool isEmpty() const;
    
    /**
     * @brief Iterate over all non-expired elements (head to tail)
     * 
     * @param callback Function called for each element. Return false to stop.
     */
    void forEach(std::function<bool(const uint8_t* data, size_t size)> callback) const;
    
    /**
     * @brief Iterate with TTL information
     */
    void forEachWithTTL(std::function<bool(const uint8_t* data, size_t size,
                                           int64_t ttl_remaining)> callback) const;
    
    /**
     * @brief Drain queue elements to a callback
     * 
     * @param callback Receives each element's data
     * @param max_elements Maximum elements to drain (0 for all)
     * @return Number of elements drained (excludes expired)
     */
    size_t drainTo(std::function<void(std::vector<uint8_t>&&)> callback, 
                   size_t max_elements = 0);
    
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
    // Get node at offset
    ShmNode* node_at_offset(int64_t offset) const;
    
    // Allocate a new node
    ShmNode* allocate_node(size_t data_size);
    
    // Free a node
    void free_node(ShmNode* node, size_t data_size);
    
    // Skip expired nodes at front
    void skip_expired_front();

    std::unique_ptr<MMapFileManager> file_manager_;
    DequeHeader* header_;
    CollectionStats stats_;
    
    // For blocking operations
    mutable IpcMutex wait_mutex_;
};

} // namespace fastcollection

#endif // FASTCOLLECTION_QUEUE_H
