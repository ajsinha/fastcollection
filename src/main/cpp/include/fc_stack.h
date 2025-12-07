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
 * @file fc_stack.h
 * @brief High-performance memory-mapped stack implementation with TTL support
 * 
 * ============================================================================
 * FASTCOLLECTION STACK - MEMORY-MAPPED LIFO STACK WITH TTL
 * ============================================================================
 * 
 * OVERVIEW:
 * ---------
 * FastStack is an ultra high-performance LIFO (Last-In-First-Out) stack that
 * stores data in memory-mapped files. It uses lock-free operations where
 * possible and supports automatic element expiration via TTL.
 * 
 * KEY FEATURES:
 * 1. PERSISTENCE: Stack survives process restarts
 * 2. IPC: Multiple processes can share the same stack
 * 3. TTL: Elements auto-expire after configurable duration
 * 4. LOCK-FREE: CAS-based push/pop for high concurrency
 * 5. ABA PREVENTION: Tagged pointers prevent ABA problem
 * 
 * STACK ARCHITECTURE:
 * -------------------
 * The stack is implemented as a singly-linked list with a top pointer:
 * 
 * +------------------+
 * | DequeHeader      |  <- front_offset (top of stack)
 * +------------------+
 *        |
 *        v
 *     [Top] -> [Node] -> [Node] -> [Node] -> null
 *       ^
 *       |
 *    push/pop
 * 
 * Lock-free operations use Compare-And-Swap (CAS) on the top pointer.
 * ABA problem is prevented using tagged pointers with version counters.
 * 
 * TTL (TIME-TO-LIVE) FEATURE:
 * ---------------------------
 * Stack elements can expire automatically:
 * 
 * - ttl_seconds = -1 (default): Element never expires
 * - ttl_seconds = 0: Element expires immediately
 * - ttl_seconds > 0: Element expires after specified seconds
 * 
 * TTL behavior in stacks:
 * - Expired elements are skipped during pop
 * - peek() returns null if top is expired
 * - search() skips expired elements
 * - Expired elements are lazily cleaned up
 * 
 * USE CASES:
 * ----------
 * 
 * 1. UNDO STACK WITH TIMEOUT:
 *    // Undo operations expire after 5 minutes
 *    stack.push(undoOperation, 300);
 *    
 *    // Undo action
 *    if (auto op = stack.pop()) {
 *        performUndo(op);
 *    }
 *    
 * 2. NAVIGATION HISTORY:
 *    // History entries expire after 30 minutes
 *    stack.push(pageUrl, 1800);
 *    
 * 3. EXPRESSION EVALUATION:
 *    // Operands with short-lived relevance
 *    stack.push(operand, 60);
 *    
 * 4. CALL STACK SIMULATION:
 *    // Track function calls with timeout
 *    stack.push(callFrame, 300);  // 5-minute timeout
 * 
 * PERFORMANCE CHARACTERISTICS:
 * ----------------------------
 * Operation     | Average Time | Notes
 * --------------|--------------|----------------------------------
 * push          | < 150ns      | O(1) - lock-free CAS
 * pop           | < 150ns      | O(1) - lock-free CAS
 * peek          | < 50ns       | O(1) - single read
 * search        | < 1µs        | O(n) - linear scan from top
 * size          | < 50ns       | O(1) - atomic counter
 * 
 * Concurrent throughput: > 10M ops/sec with 8 threads
 * 
 * LOCK-FREE IMPLEMENTATION:
 * -------------------------
 * Push operation:
 *   1. Allocate new node
 *   2. Set node->next = current top
 *   3. CAS(top, current, new_node)
 *   4. Retry if CAS fails
 * 
 * Pop operation:
 *   1. Read current top
 *   2. If empty or expired, skip
 *   3. Read top->next
 *   4. CAS(top, current, next)
 *   5. Return node data
 *   6. Retry if CAS fails
 * 
 * ABA Prevention:
 *   - Each node has a version tag
 *   - CAS compares both pointer and tag
 *   - Tag increments on each modification
 * 
 * USAGE EXAMPLES:
 * ---------------
 * 
 * C++:
 *   FastStack stack("/tmp/mystack.fc", 64*1024*1024, true);
 *   
 *   // Push with TTL
 *   std::string data = "state snapshot";
 *   stack.push((uint8_t*)data.data(), data.size(), 300);  // 5-min TTL
 *   
 *   // Pop
 *   std::vector<uint8_t> result;
 *   if (stack.pop(result)) {
 *       // Use result...
 *   }
 *   
 *   // Pop or throw
 *   try {
 *       auto data = stack.popOrThrow();
 *   } catch (FastCollectionException& e) {
 *       // Stack was empty
 *   }
 * 
 * Java (via JNI):
 *   FastCollectionStack stack = new FastCollectionStack("/tmp/mystack.fc");
 *   stack.push(state, 300);  // 5-minute TTL
 *   State s = stack.pop();
 * 
 * Python (via pybind11):
 *   s = FastStack("/tmp/mystack.fc")
 *   s.push(data, ttl=300)
 *   item = s.pop()
 */

#ifndef FASTCOLLECTION_STACK_H
#define FASTCOLLECTION_STACK_H

#include "fc_common.h"
#include "fc_serialization.h"
#include <functional>
#include <vector>

namespace fastcollection {

/**
 * @brief Ultra high-performance memory-mapped concurrent stack with TTL
 * 
 * Features:
 * - O(1) push and pop operations
 * - TTL support for automatic element expiration
 * - Lock-free push/pop via CAS operations
 * - Memory-mapped backing for persistence and IPC
 * - LIFO ordering guarantee
 * - ABA problem prevention via tagged pointers
 * 
 * Performance targets:
 * - Push: < 150ns average
 * - Pop: < 150ns average
 * - Peek: < 50ns average
 * - Concurrent throughput: > 10M ops/sec with 8 threads
 */
class FastStack {
public:
    /**
     * @brief Construct a FastStack with the given memory-mapped file
     * 
     * @param mmap_file Path to the memory-mapped file
     * @param initial_size Initial size of the memory-mapped region
     * @param create_new If true, create a new file (truncating any existing)
     * 
     * @throws FastCollectionException if file cannot be created/opened
     */
    FastStack(const std::string& mmap_file,
              size_t initial_size = DEFAULT_INITIAL_SIZE,
              bool create_new = false);
    
    ~FastStack();
    
    // Non-copyable
    FastStack(const FastStack&) = delete;
    FastStack& operator=(const FastStack&) = delete;
    
    // Movable
    FastStack(FastStack&&) noexcept;
    FastStack& operator=(FastStack&&) noexcept;
    
    // =========================================================================
    // CORE STACK OPERATIONS
    // =========================================================================
    
    /**
     * @brief Push element onto the stack
     * 
     * @param data Serialized object data
     * @param size Size of the data
     * @param ttl_seconds Time-to-live in seconds (-1 for infinite)
     * @return true if successful
     * 
     * Time Complexity: O(1) - lock-free CAS operation
     */
    bool push(const uint8_t* data, size_t size, int32_t ttl_seconds = TTL_INFINITE);
    
    /**
     * @brief Pop element from the stack
     * 
     * @param out_data Output buffer for the data
     * @return true if element was retrieved (false if stack empty)
     * 
     * Skips expired elements automatically.
     * Time Complexity: O(1) - lock-free CAS operation
     */
    bool pop(std::vector<uint8_t>& out_data);
    
    /**
     * @brief Peek at top element without removing
     * 
     * @param out_data Output buffer for the data
     * @return true if element exists and not expired
     * 
     * Time Complexity: O(1)
     */
    bool peek(std::vector<uint8_t>& out_data) const;
    
    /**
     * @brief Pop element or throw if empty
     * 
     * @return Element data
     * @throws FastCollectionException if stack is empty
     */
    std::vector<uint8_t> popOrThrow();
    
    /**
     * @brief Peek at top element or throw if empty
     * 
     * @return Element data
     * @throws FastCollectionException if stack is empty
     */
    std::vector<uint8_t> peekOrThrow() const;
    
    // =========================================================================
    // BULK OPERATIONS
    // =========================================================================
    
    /**
     * @brief Push multiple elements atomically
     * 
     * @param elements Vector of (data, size, ttl) tuples
     * @return Number of elements pushed
     * 
     * Elements are pushed in order, so last element ends up on top.
     */
    size_t pushAll(const std::vector<std::tuple<const uint8_t*, size_t, int32_t>>& elements);
    
    /**
     * @brief Pop multiple elements
     * 
     * @param out_data Output vector for elements
     * @param max_count Maximum number to pop
     * @return Number of elements popped
     */
    size_t popAll(std::vector<std::vector<uint8_t>>& out_data, size_t max_count);
    
    // =========================================================================
    // TTL OPERATIONS
    // =========================================================================
    
    /**
     * @brief Get TTL of top element
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
    // SEARCH OPERATIONS
    // =========================================================================
    
    /**
     * @brief Search for element and return distance from top
     * 
     * @param data Serialized object data
     * @param size Size of the data
     * @return 1-based distance from top (1 = top), or -1 if not found
     * 
     * Only searches non-expired elements.
     * Time Complexity: O(n)
     */
    int64_t search(const uint8_t* data, size_t size) const;
    
    /**
     * @brief Check if stack contains element (not expired)
     * 
     * @param data Data to search for
     * @param size Size of the data
     * @return true if found and not expired
     */
    bool contains(const uint8_t* data, size_t size) const {
        return search(data, size) >= 0;
    }
    
    /**
     * @brief Remove specific element (first occurrence from top)
     * 
     * @param data Data to remove
     * @param size Size of the data
     * @return true if found and removed
     * 
     * Note: This operation requires locking and is not lock-free.
     */
    bool removeElement(const uint8_t* data, size_t size);
    
    // =========================================================================
    // UTILITY OPERATIONS
    // =========================================================================
    
    /**
     * @brief Clear all elements from the stack
     */
    void clear();
    
    /**
     * @brief Get the number of non-expired elements
     */
    size_t size() const;
    
    /**
     * @brief Check if stack is empty (no non-expired elements)
     */
    bool isEmpty() const;
    
    /**
     * @brief Iterate over all non-expired elements (top to bottom)
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
    /**
     * @brief Tagged pointer for ABA prevention
     * 
     * Combines a node offset with a version tag to prevent the ABA problem
     * in lock-free operations.
     */
    struct TaggedPointer {
        int64_t offset;   // Offset to node in memory-mapped file
        uint64_t tag;     // Version counter for ABA prevention
        
        bool operator==(const TaggedPointer& other) const {
            return offset == other.offset && tag == other.tag;
        }
    };
    
    // Get node at offset
    ShmNode* node_at_offset(int64_t offset) const;
    
    // Allocate a new node
    ShmNode* allocate_node(size_t data_size);
    
    // Free a node
    void free_node(ShmNode* node, size_t data_size);

    std::unique_ptr<MMapFileManager> file_manager_;
    DequeHeader* header_;
    std::atomic<uint64_t>* aba_tag_;  // For ABA prevention
    CollectionStats stats_;
};

} // namespace fastcollection

#endif // FASTCOLLECTION_STACK_H
