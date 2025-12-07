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
 * @file fc_list.h
 * @brief High-performance memory-mapped list implementation with TTL support
 * 
 * ============================================================================
 * FASTCOLLECTION LIST - MEMORY-MAPPED DOUBLY-LINKED LIST
 * ============================================================================
 * 
 * OVERVIEW:
 * ---------
 * FastList is an ultra high-performance doubly-linked list implementation that
 * stores data in memory-mapped files. This provides several unique capabilities:
 * 
 * 1. PERSISTENCE: Data survives process restarts automatically
 * 2. IPC (Inter-Process Communication): Multiple processes can share the same list
 * 3. TTL (Time-To-Live): Elements can auto-expire after a configurable duration
 * 4. ZERO-COPY: Data is accessed directly from the memory-mapped region
 * 
 * MEMORY-MAPPED FILE ARCHITECTURE:
 * --------------------------------
 * Unlike traditional in-memory collections, FastList stores all data in a file
 * that is mapped into the process's virtual address space. The operating system
 * handles:
 * - Page faults: Loading data from disk on demand
 * - Write-back: Flushing modified pages to disk
 * - Memory pressure: Evicting unused pages when memory is low
 * 
 * File Structure:
 * +------------------+
 * | ListHeader       |  <- Fixed-size header with metadata
 * +------------------+
 * | ShmNode[0]       |  <- Variable-size nodes containing data
 * +------------------+
 * | ShmNode[1]       |
 * +------------------+
 * | ...              |
 * +------------------+
 * | Free Space       |  <- Available for new allocations
 * +------------------+
 * 
 * TTL (TIME-TO-LIVE) FEATURE:
 * ---------------------------
 * FastList supports automatic element expiration, a feature not available in
 * standard Java collections. When adding an element, you can specify:
 * 
 * - ttl_seconds = -1 (default): Element never expires
 * - ttl_seconds = 0: Element expires immediately (useful for testing)
 * - ttl_seconds > 0: Element expires after the specified seconds
 * 
 * Expired elements are:
 * - Automatically skipped during iteration
 * - Lazily cleaned up during operations
 * - Not counted in size() after expiration
 * 
 * Example:
 *   list.add(data, size, 60);  // Expires in 60 seconds
 *   list.add(data, size, -1);  // Never expires (default)
 *   list.add(data, size);      // Never expires (uses default TTL)
 * 
 * CONCURRENCY MODEL:
 * ------------------
 * - Structural changes (add, remove): Protected by global mutex
 * - Reads: Can proceed concurrently with optimistic validation
 * - TTL checks: Lock-free using atomic operations
 * 
 * PERFORMANCE CHARACTERISTICS:
 * ----------------------------
 * Operation          | Average Time | Notes
 * -------------------|--------------|----------------------------------
 * add (tail)         | < 500ns      | O(1) - direct tail pointer update
 * addFirst           | < 500ns      | O(1) - direct head pointer update
 * add (index)        | < 1µs + O(n) | O(n) - requires traversal to index
 * get (sequential)   | < 100ns      | O(1) - cached from previous access
 * get (random)       | < 1µs        | O(n) - requires traversal
 * remove             | < 500ns      | O(1) after locating node
 * contains           | < 1µs        | O(n) - full scan with hash shortcut
 * size               | < 50ns       | O(1) - atomic counter
 * 
 * USAGE EXAMPLES:
 * ---------------
 * 
 * C++:
 *   FastList list("/tmp/mylist.fc", 64*1024*1024, true);  // 64MB, create new
 *   
 *   // Add with TTL
 *   std::string data = "hello";
 *   list.add((uint8_t*)data.data(), data.size(), 300);  // 5-minute TTL
 *   
 *   // Add without TTL (never expires)
 *   list.add((uint8_t*)data.data(), data.size());
 *   
 *   // Read
 *   std::vector<uint8_t> result;
 *   if (list.get(0, result)) {
 *       // Use result...
 *   }
 * 
 * Java (via JNI):
 *   FastCollectionList list = new FastCollectionList("/tmp/mylist.fc");
 *   list.add(myObject, 300);  // 5-minute TTL
 *   list.add(myObject);       // No TTL
 * 
 * Python (via pybind11):
 *   list = FastList("/tmp/mylist.fc")
 *   list.add(data, ttl=300)  # 5-minute TTL
 *   list.add(data)           # No TTL
 */

#ifndef FASTCOLLECTION_LIST_H
#define FASTCOLLECTION_LIST_H

#include "fc_common.h"
#include "fc_serialization.h"
#include <optional>
#include <functional>

namespace fastcollection {

/**
 * @brief Ultra high-performance memory-mapped list with TTL support
 * 
 * Features:
 * - O(1) add/remove at head and tail
 * - O(n) random access (cached for sequential access patterns)
 * - TTL (Time-To-Live) support for automatic element expiration
 * - Fine-grained locking for concurrent operations
 * - Lock-free reads where possible
 * - Memory-mapped backing for persistence and IPC
 * 
 * Performance targets:
 * - Add: < 500ns average
 * - Get (sequential): < 100ns average
 * - Get (random): < 1µs average
 * - Remove: < 500ns average
 */
class FastList {
public:
    /**
     * @brief Construct a FastList with the given memory-mapped file
     * 
     * @param mmap_file Path to the memory-mapped file. The file will be created
     *                  if it doesn't exist, or opened if it does.
     * @param initial_size Initial size of the memory-mapped region in bytes.
     *                     The file will grow automatically if needed.
     * @param create_new If true, create a new file (truncating any existing).
     *                   If false, open existing file or create if not exists.
     * 
     * @throws FastCollectionException if file cannot be created/opened
     * 
     * Example:
     *   // Create new 64MB list
     *   FastList list("/tmp/mylist.fc", 64*1024*1024, true);
     *   
     *   // Open existing list (or create with default 64MB)
     *   FastList list2("/tmp/mylist.fc");
     */
    FastList(const std::string& mmap_file, 
             size_t initial_size = DEFAULT_INITIAL_SIZE,
             bool create_new = false);
    
    ~FastList();
    
    // Non-copyable (file handle cannot be shared)
    FastList(const FastList&) = delete;
    FastList& operator=(const FastList&) = delete;
    
    // Movable
    FastList(FastList&&) noexcept;
    FastList& operator=(FastList&&) noexcept;
    
    // =========================================================================
    // ADD OPERATIONS
    // =========================================================================
    
    /**
     * @brief Add an element to the end of the list
     * 
     * @param data Pointer to serialized object data
     * @param size Size of the data in bytes
     * @param ttl_seconds Time-to-live in seconds. Use TTL_INFINITE (-1) for no
     *                    expiration. Default is TTL_INFINITE.
     * @return true if successful, false on error
     * 
     * Time Complexity: O(1)
     * 
     * Example:
     *   // Add with 5-minute TTL
     *   list.add(data, size, 300);
     *   
     *   // Add without TTL (never expires)
     *   list.add(data, size);
     *   list.add(data, size, TTL_INFINITE);
     */
    bool add(const uint8_t* data, size_t size, int32_t ttl_seconds = TTL_INFINITE);
    
    /**
     * @brief Add an element at a specific index
     * 
     * @param index Position to insert at (0 = front, size() = end)
     * @param data Pointer to serialized object data
     * @param size Size of the data in bytes
     * @param ttl_seconds Time-to-live in seconds (-1 for infinite)
     * @return true if successful, false if index out of bounds
     * 
     * Time Complexity: O(n) - requires traversal to index
     * 
     * Example:
     *   // Insert at position 5 with 1-hour TTL
     *   list.add(5, data, size, 3600);
     */
    bool add(size_t index, const uint8_t* data, size_t size, int32_t ttl_seconds = TTL_INFINITE);
    
    /**
     * @brief Add an element to the front of the list
     * 
     * @param data Pointer to serialized object data
     * @param size Size of the data in bytes
     * @param ttl_seconds Time-to-live in seconds (-1 for infinite)
     * @return true if successful
     * 
     * Time Complexity: O(1)
     */
    bool addFirst(const uint8_t* data, size_t size, int32_t ttl_seconds = TTL_INFINITE);
    
    // =========================================================================
    // GET OPERATIONS
    // =========================================================================
    
    /**
     * @brief Get element at the specified index
     * 
     * @param index Index of the element (0-based)
     * @param out_data Output buffer for the data
     * @return true if element found and not expired, false otherwise
     * 
     * Time Complexity: O(n) for random access, O(1) for sequential access
     * 
     * Note: Expired elements are skipped during index calculation.
     */
    bool get(size_t index, std::vector<uint8_t>& out_data) const;
    
    /**
     * @brief Get the first element
     * 
     * @param out_data Output buffer for the data
     * @return true if list is not empty and first element not expired
     * 
     * Time Complexity: O(1)
     */
    bool getFirst(std::vector<uint8_t>& out_data) const;
    
    /**
     * @brief Get the last element
     * 
     * @param out_data Output buffer for the data
     * @return true if list is not empty and last element not expired
     * 
     * Time Complexity: O(1)
     */
    bool getLast(std::vector<uint8_t>& out_data) const;
    
    /**
     * @brief Get remaining TTL for element at index
     * 
     * @param index Index of the element
     * @return Remaining seconds until expiration, -1 if infinite, 0 if expired
     */
    int64_t getTTL(size_t index) const;
    
    // =========================================================================
    // SET OPERATIONS
    // =========================================================================
    
    /**
     * @brief Set element at the specified index
     * 
     * @param index Index of the element
     * @param data New serialized data
     * @param size Size of the data
     * @param ttl_seconds New TTL in seconds (-1 to keep existing TTL)
     * @return true if successful
     * 
     * Time Complexity: O(n) - requires traversal to index
     */
    bool set(size_t index, const uint8_t* data, size_t size, int32_t ttl_seconds = TTL_INFINITE);
    
    /**
     * @brief Update TTL for element at index without changing data
     * 
     * @param index Index of the element
     * @param ttl_seconds New TTL in seconds (-1 for infinite)
     * @return true if successful
     */
    bool setTTL(size_t index, int32_t ttl_seconds);
    
    // =========================================================================
    // REMOVE OPERATIONS
    // =========================================================================
    
    /**
     * @brief Remove element at the specified index
     * 
     * @param index Index of the element to remove
     * @param out_data Optional output buffer for removed data
     * @return true if element was removed
     * 
     * Time Complexity: O(n) - requires traversal to index
     */
    bool remove(size_t index, std::vector<uint8_t>* out_data = nullptr);
    
    /**
     * @brief Remove the first element
     * 
     * @param out_data Optional output buffer for removed data
     * @return true if element was removed
     * 
     * Time Complexity: O(1)
     */
    bool removeFirst(std::vector<uint8_t>* out_data = nullptr);
    
    /**
     * @brief Remove the last element
     * 
     * @param out_data Optional output buffer for removed data
     * @return true if element was removed
     * 
     * Time Complexity: O(1)
     */
    bool removeLast(std::vector<uint8_t>* out_data = nullptr);
    
    /**
     * @brief Remove first occurrence of the specified element
     * 
     * @param data Data to search for
     * @param size Size of the data
     * @return true if element was found and removed
     * 
     * Time Complexity: O(n) - requires full scan
     */
    bool removeElement(const uint8_t* data, size_t size);
    
    /**
     * @brief Remove all expired elements
     * 
     * @return Number of elements removed
     * 
     * This is called automatically during other operations but can be
     * invoked manually for maintenance.
     */
    size_t removeExpired();
    
    // =========================================================================
    // SEARCH OPERATIONS
    // =========================================================================
    
    /**
     * @brief Check if list contains the specified element
     * 
     * @param data Data to search for
     * @param size Size of the data
     * @return true if element is found and not expired
     * 
     * Time Complexity: O(n)
     */
    bool contains(const uint8_t* data, size_t size) const;
    
    /**
     * @brief Find the index of the first occurrence of an element
     * 
     * @param data Data to search for
     * @param size Size of the data
     * @return Index of element, or -1 if not found
     * 
     * Time Complexity: O(n)
     */
    int64_t indexOf(const uint8_t* data, size_t size) const;
    
    /**
     * @brief Find the index of the last occurrence of an element
     * 
     * @param data Data to search for
     * @param size Size of the data
     * @return Index of element, or -1 if not found
     * 
     * Time Complexity: O(n)
     */
    int64_t lastIndexOf(const uint8_t* data, size_t size) const;
    
    // =========================================================================
    // UTILITY OPERATIONS
    // =========================================================================
    
    /**
     * @brief Clear all elements from the list
     * 
     * This removes all elements including those with remaining TTL.
     */
    void clear();
    
    /**
     * @brief Get the number of non-expired elements in the list
     * 
     * Note: This may trigger lazy cleanup of expired elements.
     */
    size_t size() const;
    
    /**
     * @brief Check if list is empty (no non-expired elements)
     */
    bool isEmpty() const;
    
    /**
     * @brief Iterate over all non-expired elements
     * 
     * @param callback Function called for each element.
     *                 Parameters: data pointer, data size, index
     *                 Return false to stop iteration.
     * 
     * Example:
     *   list.forEach([](const uint8_t* data, size_t size, size_t idx) {
     *       std::cout << "Element " << idx << ": " << size << " bytes\n";
     *       return true;  // Continue iteration
     *   });
     */
    void forEach(std::function<bool(const uint8_t* data, size_t size, size_t index)> callback) const;
    
    /**
     * @brief Iterate with TTL information
     * 
     * @param callback Function receiving data, size, index, and remaining TTL
     */
    void forEachWithTTL(std::function<bool(const uint8_t* data, size_t size, 
                                           size_t index, int64_t ttl_remaining)> callback) const;
    
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
     * 
     * Forces all pending writes to be persisted to the file.
     */
    void flush();

private:
    // Get node at offset (with prefetching)
    ShmNode* node_at_offset(int64_t offset) const;
    
    // Get node at index (caches recent lookups), skips expired
    ShmNode* node_at_index(size_t index) const;
    
    // Allocate a new node
    ShmNode* allocate_node(size_t data_size);
    
    // Free a node
    void free_node(ShmNode* node, size_t data_size);
    
    // Link a node into the list
    void link_node(ShmNode* node, ShmNode* prev, ShmNode* next);
    
    // Unlink a node from the list
    void unlink_node(ShmNode* node);
    
    // Check and remove expired nodes lazily
    void lazy_cleanup_expired() const;

    std::unique_ptr<MMapFileManager> file_manager_;
    ListHeader* header_;
    CollectionStats stats_;
    
    // Cache for sequential access optimization
    mutable struct {
        size_t last_index = SIZE_MAX;
        int64_t last_offset = -1;
    } access_cache_;
};

} // namespace fastcollection

#endif // FASTCOLLECTION_LIST_H
