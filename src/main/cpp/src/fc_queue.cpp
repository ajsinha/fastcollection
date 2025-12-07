/**
 * Copyright © 2025-2030, All Rights Reserved
 * Ashutosh Sinha | Email: ajsinha@gmail.com
 * 
 * Patent Pending
 * 
 * @file fc_queue.cpp
 * @brief Implementation of high-performance memory-mapped queue with TTL
 */

#include "fc_queue.h"
#include <cstring>
#include <thread>
#include <chrono>

namespace fastcollection {

using IpcScopedLock = bip::scoped_lock<IpcSharedMutex>;

FastQueue::FastQueue(const std::string& mmap_file,
                     size_t initial_size,
                     bool create_new)
    : file_manager_(std::make_unique<MMapFileManager>(mmap_file, initial_size, create_new)) {
    
    auto result = file_manager_->find<DequeHeader>("queue_header");
    
    if (result.first) {
        header_ = result.first;
        if (!header_->is_valid()) {
            throw FastCollectionException(
                FastCollectionException::ErrorCode::INTERNAL_ERROR,
                "Invalid queue header in file"
            );
        }
    } else {
        header_ = file_manager_->find_or_construct<DequeHeader>("queue_header")();
    }
    
    stats_.size.store(header_->size.load(), std::memory_order_relaxed);
}

FastQueue::~FastQueue() {
    if (file_manager_) {
        flush();
    }
}

FastQueue::FastQueue(FastQueue&& other) noexcept
    : file_manager_(std::move(other.file_manager_))
    , header_(other.header_) {
    other.header_ = nullptr;
}

FastQueue& FastQueue::operator=(FastQueue&& other) noexcept {
    if (this != &other) {
        file_manager_ = std::move(other.file_manager_);
        header_ = other.header_;
        other.header_ = nullptr;
    }
    return *this;
}

ShmNode* FastQueue::node_at_offset(int64_t offset) const {
    if (offset < 0) return nullptr;
    
    void* base = file_manager_->segment_manager();
    return reinterpret_cast<ShmNode*>(static_cast<uint8_t*>(base) + offset);
}

ShmNode* FastQueue::allocate_node(size_t data_size) {
    size_t total = ShmNode::total_size(data_size);
    void* mem = file_manager_->allocate(total);
    if (!mem) {
        throw FastCollectionException(
            FastCollectionException::ErrorCode::MEMORY_ALLOCATION_FAILED,
            "Failed to allocate node"
        );
    }
    return new(mem) ShmNode();
}

void FastQueue::free_node(ShmNode* node, size_t data_size) {
    if (node) {
        file_manager_->deallocate(node);
    }
}

void FastQueue::skip_expired_front() {
    // Skip expired nodes at front (must be called with lock held)
    void* base = file_manager_->segment_manager();
    
    while (true) {
        int64_t front = header_->front_offset.load(std::memory_order_acquire);
        if (front < 0) break;
        
        ShmNode* node = node_at_offset(front);
        if (!node) break;
        
        if (!node->entry.is_expired()) break;
        
        // Remove expired node
        int64_t next = node->next_offset.load(std::memory_order_acquire);
        header_->front_offset.store(next, std::memory_order_release);
        
        if (next >= 0) {
            ShmNode* next_node = node_at_offset(next);
            next_node->prev_offset.store(ShmNode::NULL_OFFSET, std::memory_order_release);
        } else {
            header_->back_offset.store(ShmNode::NULL_OFFSET, std::memory_order_release);
        }
        
        size_t data_size = node->entry.data_size;
        node->entry.mark_deleted();
        free_node(node, data_size);
        
        header_->size.fetch_sub(1, std::memory_order_acq_rel);
        stats_.size.fetch_sub(1, std::memory_order_relaxed);
    }
}

bool FastQueue::offer(const uint8_t* data, size_t size, int32_t ttl_seconds) {
    if (!data || size == 0) return false;
    
    IpcScopedLock lock(header_->global_mutex);
    
    void* base = file_manager_->segment_manager();
    
    ShmNode* node = allocate_node(size);
    SerializationUtil::copy_to_node(node, data, size, ttl_seconds);
    
    int64_t node_offset = static_cast<uint8_t*>(static_cast<void*>(node)) - 
                          static_cast<uint8_t*>(base);
    
    // Add to back
    int64_t back = header_->back_offset.load(std::memory_order_acquire);
    
    node->prev_offset.store(back, std::memory_order_release);
    node->next_offset.store(ShmNode::NULL_OFFSET, std::memory_order_release);
    
    if (back >= 0) {
        ShmNode* back_node = node_at_offset(back);
        back_node->next_offset.store(node_offset, std::memory_order_release);
    } else {
        header_->front_offset.store(node_offset, std::memory_order_release);
    }
    
    header_->back_offset.store(node_offset, std::memory_order_release);
    header_->size.fetch_add(1, std::memory_order_acq_rel);
    header_->modified_at = current_timestamp_ns();
    
    stats_.size.fetch_add(1, std::memory_order_relaxed);
    stats_.write_count.fetch_add(1, std::memory_order_relaxed);
    
    return true;
}

bool FastQueue::poll(std::vector<uint8_t>& out_data) {
    IpcScopedLock lock(header_->global_mutex);
    
    // Skip expired nodes
    skip_expired_front();
    
    int64_t front = header_->front_offset.load(std::memory_order_acquire);
    if (front < 0) {
        stats_.miss_count.fetch_add(1, std::memory_order_relaxed);
        return false;
    }
    
    ShmNode* node = node_at_offset(front);
    if (!node || !node->entry.is_alive()) {
        stats_.miss_count.fetch_add(1, std::memory_order_relaxed);
        return false;
    }
    
    out_data = SerializationUtil::copy_from_node(node);
    
    // Remove from front
    int64_t next = node->next_offset.load(std::memory_order_acquire);
    header_->front_offset.store(next, std::memory_order_release);
    
    if (next >= 0) {
        ShmNode* next_node = node_at_offset(next);
        next_node->prev_offset.store(ShmNode::NULL_OFFSET, std::memory_order_release);
    } else {
        header_->back_offset.store(ShmNode::NULL_OFFSET, std::memory_order_release);
    }
    
    size_t data_size = node->entry.data_size;
    node->entry.mark_deleted();
    free_node(node, data_size);
    
    header_->size.fetch_sub(1, std::memory_order_acq_rel);
    header_->modified_at = current_timestamp_ns();
    
    stats_.size.fetch_sub(1, std::memory_order_relaxed);
    stats_.read_count.fetch_add(1, std::memory_order_relaxed);
    stats_.hit_count.fetch_add(1, std::memory_order_relaxed);
    
    return true;
}

std::vector<uint8_t> FastQueue::remove() {
    std::vector<uint8_t> result;
    if (!poll(result)) {
        throw FastCollectionException(
            FastCollectionException::ErrorCode::NOT_FOUND,
            "Queue is empty"
        );
    }
    return result;
}

bool FastQueue::peek(std::vector<uint8_t>& out_data) const {
    IpcScopedLock lock(const_cast<IpcSharedMutex&>(header_->global_mutex));
    
    // Skip expired at front
    const_cast<FastQueue*>(this)->skip_expired_front();
    
    int64_t front = header_->front_offset.load(std::memory_order_acquire);
    if (front < 0) {
        const_cast<CollectionStats&>(stats_).miss_count.fetch_add(1, std::memory_order_relaxed);
        return false;
    }
    
    ShmNode* node = node_at_offset(front);
    if (!node || !node->entry.is_alive()) {
        const_cast<CollectionStats&>(stats_).miss_count.fetch_add(1, std::memory_order_relaxed);
        return false;
    }
    
    out_data = SerializationUtil::copy_from_node(node);
    
    const_cast<CollectionStats&>(stats_).read_count.fetch_add(1, std::memory_order_relaxed);
    const_cast<CollectionStats&>(stats_).hit_count.fetch_add(1, std::memory_order_relaxed);
    
    return true;
}

std::vector<uint8_t> FastQueue::element() const {
    std::vector<uint8_t> result;
    if (!peek(result)) {
        throw FastCollectionException(
            FastCollectionException::ErrorCode::NOT_FOUND,
            "Queue is empty"
        );
    }
    return result;
}

void FastQueue::put(const uint8_t* data, size_t size, int32_t ttl_seconds) {
    // Unbounded queue - always succeeds
    offer(data, size, ttl_seconds);
}

bool FastQueue::offer(const uint8_t* data, size_t size, uint32_t timeout_ms, 
                      int32_t ttl_seconds) {
    // Unbounded queue - always succeeds immediately
    return offer(data, size, ttl_seconds);
}

std::vector<uint8_t> FastQueue::take() {
    std::vector<uint8_t> result;
    
    while (true) {
        if (poll(result)) {
            return result;
        }
        // Wait and retry
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

bool FastQueue::poll(std::vector<uint8_t>& out_data, uint32_t timeout_ms) {
    auto deadline = std::chrono::steady_clock::now() + 
                    std::chrono::milliseconds(timeout_ms);
    
    while (std::chrono::steady_clock::now() < deadline) {
        if (poll(out_data)) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    return false;
}

bool FastQueue::offerFirst(const uint8_t* data, size_t size, int32_t ttl_seconds) {
    if (!data || size == 0) return false;
    
    IpcScopedLock lock(header_->global_mutex);
    
    void* base = file_manager_->segment_manager();
    
    ShmNode* node = allocate_node(size);
    SerializationUtil::copy_to_node(node, data, size, ttl_seconds);
    
    int64_t node_offset = static_cast<uint8_t*>(static_cast<void*>(node)) - 
                          static_cast<uint8_t*>(base);
    
    // Add to front
    int64_t front = header_->front_offset.load(std::memory_order_acquire);
    
    node->next_offset.store(front, std::memory_order_release);
    node->prev_offset.store(ShmNode::NULL_OFFSET, std::memory_order_release);
    
    if (front >= 0) {
        ShmNode* front_node = node_at_offset(front);
        front_node->prev_offset.store(node_offset, std::memory_order_release);
    } else {
        header_->back_offset.store(node_offset, std::memory_order_release);
    }
    
    header_->front_offset.store(node_offset, std::memory_order_release);
    header_->size.fetch_add(1, std::memory_order_acq_rel);
    header_->modified_at = current_timestamp_ns();
    
    stats_.size.fetch_add(1, std::memory_order_relaxed);
    stats_.write_count.fetch_add(1, std::memory_order_relaxed);
    
    return true;
}

bool FastQueue::pollLast(std::vector<uint8_t>& out_data) {
    IpcScopedLock lock(header_->global_mutex);
    
    int64_t back = header_->back_offset.load(std::memory_order_acquire);
    
    // Skip expired from back
    while (back >= 0) {
        ShmNode* node = node_at_offset(back);
        if (!node) break;
        
        if (node->entry.is_alive()) break;
        
        // Remove expired node
        int64_t prev = node->prev_offset.load(std::memory_order_acquire);
        header_->back_offset.store(prev, std::memory_order_release);
        
        if (prev >= 0) {
            ShmNode* prev_node = node_at_offset(prev);
            prev_node->next_offset.store(ShmNode::NULL_OFFSET, std::memory_order_release);
        } else {
            header_->front_offset.store(ShmNode::NULL_OFFSET, std::memory_order_release);
        }
        
        size_t data_size = node->entry.data_size;
        node->entry.mark_deleted();
        free_node(node, data_size);
        
        header_->size.fetch_sub(1, std::memory_order_acq_rel);
        stats_.size.fetch_sub(1, std::memory_order_relaxed);
        
        back = prev;
    }
    
    if (back < 0) {
        stats_.miss_count.fetch_add(1, std::memory_order_relaxed);
        return false;
    }
    
    ShmNode* node = node_at_offset(back);
    if (!node || !node->entry.is_alive()) {
        stats_.miss_count.fetch_add(1, std::memory_order_relaxed);
        return false;
    }
    
    out_data = SerializationUtil::copy_from_node(node);
    
    // Remove from back
    int64_t prev = node->prev_offset.load(std::memory_order_acquire);
    header_->back_offset.store(prev, std::memory_order_release);
    
    if (prev >= 0) {
        ShmNode* prev_node = node_at_offset(prev);
        prev_node->next_offset.store(ShmNode::NULL_OFFSET, std::memory_order_release);
    } else {
        header_->front_offset.store(ShmNode::NULL_OFFSET, std::memory_order_release);
    }
    
    size_t data_size = node->entry.data_size;
    node->entry.mark_deleted();
    free_node(node, data_size);
    
    header_->size.fetch_sub(1, std::memory_order_acq_rel);
    header_->modified_at = current_timestamp_ns();
    
    stats_.size.fetch_sub(1, std::memory_order_relaxed);
    stats_.read_count.fetch_add(1, std::memory_order_relaxed);
    stats_.hit_count.fetch_add(1, std::memory_order_relaxed);
    
    return true;
}

bool FastQueue::peekLast(std::vector<uint8_t>& out_data) const {
    IpcScopedLock lock(const_cast<IpcSharedMutex&>(header_->global_mutex));
    
    int64_t back = header_->back_offset.load(std::memory_order_acquire);
    
    // Skip expired from back
    while (back >= 0) {
        ShmNode* node = node_at_offset(back);
        if (!node) break;
        if (node->entry.is_alive()) break;
        back = node->prev_offset.load(std::memory_order_acquire);
    }
    
    if (back < 0) {
        const_cast<CollectionStats&>(stats_).miss_count.fetch_add(1, std::memory_order_relaxed);
        return false;
    }
    
    ShmNode* node = node_at_offset(back);
    if (!node || !node->entry.is_alive()) {
        const_cast<CollectionStats&>(stats_).miss_count.fetch_add(1, std::memory_order_relaxed);
        return false;
    }
    
    out_data = SerializationUtil::copy_from_node(node);
    
    const_cast<CollectionStats&>(stats_).read_count.fetch_add(1, std::memory_order_relaxed);
    const_cast<CollectionStats&>(stats_).hit_count.fetch_add(1, std::memory_order_relaxed);
    
    return true;
}

int64_t FastQueue::peekTTL() const {
    IpcScopedLock lock(const_cast<IpcSharedMutex&>(header_->global_mutex));
    
    int64_t front = header_->front_offset.load(std::memory_order_acquire);
    
    while (front >= 0) {
        ShmNode* node = node_at_offset(front);
        if (!node) break;
        if (node->entry.is_alive()) {
            return node->entry.remaining_ttl_seconds();
        }
        front = node->next_offset.load(std::memory_order_acquire);
    }
    
    return 0;
}

size_t FastQueue::removeExpired() {
    IpcScopedLock lock(header_->global_mutex);
    
    size_t removed = 0;
    void* base = file_manager_->segment_manager();
    
    int64_t current = header_->front_offset.load(std::memory_order_acquire);
    
    while (current >= 0) {
        ShmNode* node = node_at_offset(current);
        if (!node) break;
        
        int64_t next = node->next_offset.load(std::memory_order_acquire);
        
        if (node->entry.is_expired()) {
            int64_t prev = node->prev_offset.load(std::memory_order_acquire);
            
            if (prev >= 0) {
                ShmNode* prev_node = node_at_offset(prev);
                prev_node->next_offset.store(next, std::memory_order_release);
            } else {
                header_->front_offset.store(next, std::memory_order_release);
            }
            
            if (next >= 0) {
                ShmNode* next_node = node_at_offset(next);
                next_node->prev_offset.store(prev, std::memory_order_release);
            } else {
                header_->back_offset.store(prev, std::memory_order_release);
            }
            
            size_t data_size = node->entry.data_size;
            node->entry.mark_deleted();
            free_node(node, data_size);
            
            header_->size.fetch_sub(1, std::memory_order_acq_rel);
            stats_.size.fetch_sub(1, std::memory_order_relaxed);
            removed++;
        }
        
        current = next;
    }
    
    if (removed > 0) {
        header_->modified_at = current_timestamp_ns();
    }
    
    return removed;
}

bool FastQueue::contains(const uint8_t* data, size_t size) const {
    if (!data || size == 0) return false;
    
    uint32_t hash = compute_hash(data, size);
    
    IpcScopedLock lock(const_cast<IpcSharedMutex&>(header_->global_mutex));
    
    int64_t current = header_->front_offset.load(std::memory_order_acquire);
    
    while (current >= 0) {
        ShmNode* node = node_at_offset(current);
        if (!node) break;
        
        if (node->entry.is_alive() &&
            node->entry.hash_code == hash &&
            node->entry.data_size == size &&
            std::memcmp(node->data, data, size) == 0) {
            return true;
        }
        
        current = node->next_offset.load(std::memory_order_acquire);
    }
    
    return false;
}

bool FastQueue::removeElement(const uint8_t* data, size_t size) {
    if (!data || size == 0) return false;
    
    uint32_t hash = compute_hash(data, size);
    
    IpcScopedLock lock(header_->global_mutex);
    
    void* base = file_manager_->segment_manager();
    int64_t current = header_->front_offset.load(std::memory_order_acquire);
    
    while (current >= 0) {
        ShmNode* node = node_at_offset(current);
        if (!node) break;
        
        if (node->entry.is_alive() &&
            node->entry.hash_code == hash &&
            node->entry.data_size == size &&
            std::memcmp(node->data, data, size) == 0) {
            
            int64_t prev = node->prev_offset.load(std::memory_order_acquire);
            int64_t next = node->next_offset.load(std::memory_order_acquire);
            
            if (prev >= 0) {
                ShmNode* prev_node = node_at_offset(prev);
                prev_node->next_offset.store(next, std::memory_order_release);
            } else {
                header_->front_offset.store(next, std::memory_order_release);
            }
            
            if (next >= 0) {
                ShmNode* next_node = node_at_offset(next);
                next_node->prev_offset.store(prev, std::memory_order_release);
            } else {
                header_->back_offset.store(prev, std::memory_order_release);
            }
            
            size_t data_size = node->entry.data_size;
            node->entry.mark_deleted();
            free_node(node, data_size);
            
            header_->size.fetch_sub(1, std::memory_order_acq_rel);
            header_->modified_at = current_timestamp_ns();
            stats_.size.fetch_sub(1, std::memory_order_relaxed);
            
            return true;
        }
        
        current = node->next_offset.load(std::memory_order_acquire);
    }
    
    return false;
}

void FastQueue::clear() {
    IpcScopedLock lock(header_->global_mutex);
    
    int64_t current = header_->front_offset.load(std::memory_order_acquire);
    
    while (current >= 0) {
        ShmNode* node = node_at_offset(current);
        if (!node) break;
        
        int64_t next = node->next_offset.load(std::memory_order_acquire);
        
        size_t data_size = node->entry.data_size;
        node->entry.mark_deleted();
        free_node(node, data_size);
        
        current = next;
    }
    
    header_->front_offset.store(ShmNode::NULL_OFFSET, std::memory_order_release);
    header_->back_offset.store(ShmNode::NULL_OFFSET, std::memory_order_release);
    header_->size.store(0, std::memory_order_release);
    header_->modified_at = current_timestamp_ns();
    
    stats_.size.store(0, std::memory_order_relaxed);
}

size_t FastQueue::size() const {
    IpcScopedLock lock(const_cast<IpcSharedMutex&>(header_->global_mutex));
    
    size_t alive = 0;
    int64_t current = header_->front_offset.load(std::memory_order_acquire);
    
    while (current >= 0) {
        ShmNode* node = node_at_offset(current);
        if (!node) break;
        if (node->entry.is_alive()) alive++;
        current = node->next_offset.load(std::memory_order_acquire);
    }
    
    return alive;
}

bool FastQueue::isEmpty() const {
    return size() == 0;
}

void FastQueue::forEach(std::function<bool(const uint8_t* data, size_t size)> callback) const {
    IpcScopedLock lock(const_cast<IpcSharedMutex&>(header_->global_mutex));
    
    int64_t current = header_->front_offset.load(std::memory_order_acquire);
    
    while (current >= 0) {
        ShmNode* node = node_at_offset(current);
        if (!node) break;
        
        if (node->entry.is_alive()) {
            if (!callback(node->data, node->entry.data_size)) {
                break;
            }
        }
        
        current = node->next_offset.load(std::memory_order_acquire);
    }
}

void FastQueue::forEachWithTTL(std::function<bool(const uint8_t* data, size_t size,
                                                   int64_t ttl_remaining)> callback) const {
    IpcScopedLock lock(const_cast<IpcSharedMutex&>(header_->global_mutex));
    
    int64_t current = header_->front_offset.load(std::memory_order_acquire);
    
    while (current >= 0) {
        ShmNode* node = node_at_offset(current);
        if (!node) break;
        
        if (node->entry.is_alive()) {
            int64_t ttl = node->entry.remaining_ttl_seconds();
            if (!callback(node->data, node->entry.data_size, ttl)) {
                break;
            }
        }
        
        current = node->next_offset.load(std::memory_order_acquire);
    }
}

size_t FastQueue::drainTo(std::function<void(std::vector<uint8_t>&&)> callback, 
                          size_t max_elements) {
    IpcScopedLock lock(header_->global_mutex);
    
    size_t drained = 0;
    size_t limit = (max_elements == 0) ? SIZE_MAX : max_elements;
    
    while (drained < limit) {
        skip_expired_front();
        
        int64_t front = header_->front_offset.load(std::memory_order_acquire);
        if (front < 0) break;
        
        ShmNode* node = node_at_offset(front);
        if (!node || !node->entry.is_alive()) break;
        
        std::vector<uint8_t> data = SerializationUtil::copy_from_node(node);
        
        // Remove
        int64_t next = node->next_offset.load(std::memory_order_acquire);
        header_->front_offset.store(next, std::memory_order_release);
        
        if (next >= 0) {
            ShmNode* next_node = node_at_offset(next);
            next_node->prev_offset.store(ShmNode::NULL_OFFSET, std::memory_order_release);
        } else {
            header_->back_offset.store(ShmNode::NULL_OFFSET, std::memory_order_release);
        }
        
        size_t data_size = node->entry.data_size;
        node->entry.mark_deleted();
        free_node(node, data_size);
        
        header_->size.fetch_sub(1, std::memory_order_acq_rel);
        stats_.size.fetch_sub(1, std::memory_order_relaxed);
        
        callback(std::move(data));
        drained++;
    }
    
    if (drained > 0) {
        header_->modified_at = current_timestamp_ns();
        stats_.read_count.fetch_add(drained, std::memory_order_relaxed);
    }
    
    return drained;
}

void FastQueue::flush() {
    file_manager_->flush();
}

} // namespace fastcollection
