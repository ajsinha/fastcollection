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
 * @file fc_list.cpp
 * @brief Implementation of high-performance memory-mapped list with TTL
 */

#include "fc_list.h"
#include <cstring>

namespace fastcollection {

// Scoped lock type alias for cleaner code
using IpcScopedLock = bip::scoped_lock<IpcSharedMutex>;

FastList::FastList(const std::string& mmap_file, 
                   size_t initial_size,
                   bool create_new)
    : file_manager_(std::make_unique<MMapFileManager>(mmap_file, initial_size, create_new)) {
    
    // Find or create the list header
    auto result = file_manager_->find<ListHeader>("list_header");
    
    if (result.first) {
        header_ = result.first;
        if (!header_->is_valid()) {
            throw FastCollectionException(
                FastCollectionException::ErrorCode::INTERNAL_ERROR,
                "Invalid list header in file"
            );
        }
    } else {
        // Create new header
        header_ = file_manager_->find_or_construct<ListHeader>("list_header");
    }
    
    stats_.size.store(header_->size.load(), std::memory_order_relaxed);
}

FastList::~FastList() {
    if (file_manager_) {
        flush();
    }
}

FastList::FastList(FastList&& other) noexcept
    : file_manager_(std::move(other.file_manager_))
    , header_(other.header_)
    , access_cache_(other.access_cache_) {
    other.header_ = nullptr;
}

FastList& FastList::operator=(FastList&& other) noexcept {
    if (this != &other) {
        file_manager_ = std::move(other.file_manager_);
        header_ = other.header_;
        access_cache_ = other.access_cache_;
        other.header_ = nullptr;
    }
    return *this;
}

ShmNode* FastList::node_at_offset(int64_t offset) const {
    if (offset < 0) return nullptr;
    
    void* base = file_manager_->segment_manager();
    ShmNode* node = reinterpret_cast<ShmNode*>(
        static_cast<uint8_t*>(base) + offset
    );
    
    // Prefetch next node for linked traversal
    if (node && node->next_offset.load(std::memory_order_relaxed) >= 0) {
        prefetch_read(static_cast<uint8_t*>(base) + 
                     node->next_offset.load(std::memory_order_relaxed));
    }
    
    return node;
}

ShmNode* FastList::node_at_index(size_t index) const {
    void* base = file_manager_->segment_manager();
    
    // Check cache for sequential access optimization
    if (access_cache_.last_index != SIZE_MAX && 
        access_cache_.last_offset >= 0) {
        
        // Sequential forward access
        if (index == access_cache_.last_index + 1) {
            ShmNode* cached = node_at_offset(access_cache_.last_offset);
            if (cached) {
                int64_t next = cached->next_offset.load(std::memory_order_acquire);
                ShmNode* node = node_at_offset(next);
                // Skip expired nodes
                while (node && node->entry.is_expired()) {
                    next = node->next_offset.load(std::memory_order_acquire);
                    node = node_at_offset(next);
                }
                if (node && node->entry.is_alive()) {
                    access_cache_.last_index = index;
                    access_cache_.last_offset = next;
                    return node;
                }
            }
        }
        
        // Sequential backward access
        if (index == access_cache_.last_index - 1 && access_cache_.last_index > 0) {
            ShmNode* cached = node_at_offset(access_cache_.last_offset);
            if (cached) {
                int64_t prev = cached->prev_offset.load(std::memory_order_acquire);
                ShmNode* node = node_at_offset(prev);
                // Skip expired nodes
                while (node && node->entry.is_expired()) {
                    prev = node->prev_offset.load(std::memory_order_acquire);
                    node = node_at_offset(prev);
                }
                if (node && node->entry.is_alive()) {
                    access_cache_.last_index = index;
                    access_cache_.last_offset = prev;
                    return node;
                }
            }
        }
    }
    
    // Full traversal from optimal end, skipping expired nodes
    size_t list_size = header_->size.load(std::memory_order_acquire);
    
    if (index < list_size / 2) {
        // Traverse from head
        int64_t current = header_->head_offset.load(std::memory_order_acquire);
        size_t live_index = 0;
        
        while (current >= 0) {
            ShmNode* node = node_at_offset(current);
            if (!node) break;
            
            if (node->entry.is_alive()) {
                if (live_index == index) {
                    access_cache_.last_index = index;
                    access_cache_.last_offset = current;
                    return node;
                }
                live_index++;
            }
            current = node->next_offset.load(std::memory_order_acquire);
        }
    } else {
        // Traverse from tail
        int64_t current = header_->tail_offset.load(std::memory_order_acquire);
        size_t live_index = list_size - 1;
        
        while (current >= 0) {
            ShmNode* node = node_at_offset(current);
            if (!node) break;
            
            if (node->entry.is_alive()) {
                if (live_index == index) {
                    access_cache_.last_index = index;
                    access_cache_.last_offset = current;
                    return node;
                }
                live_index--;
            }
            current = node->prev_offset.load(std::memory_order_acquire);
        }
    }
    
    return nullptr;
}

ShmNode* FastList::allocate_node(size_t data_size) {
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

void FastList::free_node(ShmNode* node, size_t data_size) {
    if (node) {
        file_manager_->deallocate(node);
    }
}

void FastList::link_node(ShmNode* node, ShmNode* prev, ShmNode* next) {
    void* base = file_manager_->segment_manager();
    int64_t node_offset = static_cast<uint8_t*>(static_cast<void*>(node)) - 
                          static_cast<uint8_t*>(base);
    
    if (prev) {
        int64_t prev_offset = static_cast<uint8_t*>(static_cast<void*>(prev)) - 
                              static_cast<uint8_t*>(base);
        node->prev_offset.store(prev_offset, std::memory_order_release);
        prev->next_offset.store(node_offset, std::memory_order_release);
    } else {
        node->prev_offset.store(ShmNode::NULL_OFFSET, std::memory_order_release);
        header_->head_offset.store(node_offset, std::memory_order_release);
    }
    
    if (next) {
        int64_t next_offset = static_cast<uint8_t*>(static_cast<void*>(next)) - 
                              static_cast<uint8_t*>(base);
        node->next_offset.store(next_offset, std::memory_order_release);
        next->prev_offset.store(node_offset, std::memory_order_release);
    } else {
        node->next_offset.store(ShmNode::NULL_OFFSET, std::memory_order_release);
        header_->tail_offset.store(node_offset, std::memory_order_release);
    }
}

void FastList::unlink_node(ShmNode* node) {
    void* base = file_manager_->segment_manager();
    
    int64_t prev = node->prev_offset.load(std::memory_order_acquire);
    int64_t next = node->next_offset.load(std::memory_order_acquire);
    
    if (prev >= 0) {
        ShmNode* prev_node = node_at_offset(prev);
        prev_node->next_offset.store(next, std::memory_order_release);
    } else {
        header_->head_offset.store(next, std::memory_order_release);
    }
    
    if (next >= 0) {
        ShmNode* next_node = node_at_offset(next);
        next_node->prev_offset.store(prev, std::memory_order_release);
    } else {
        header_->tail_offset.store(prev, std::memory_order_release);
    }
    
    // Invalidate cache
    access_cache_.last_index = SIZE_MAX;
    access_cache_.last_offset = -1;
}

bool FastList::add(const uint8_t* data, size_t size, int32_t ttl_seconds) {
    if (!data || size == 0) return false;
    
    IpcScopedLock lock(header_->global_mutex);
    
    void* base = file_manager_->segment_manager();
    
    // Allocate new node
    ShmNode* node = allocate_node(size);
    
    // Copy data with TTL
    SerializationUtil::copy_to_node(node, data, size, ttl_seconds);
    
    // Link at tail
    int64_t tail = header_->tail_offset.load(std::memory_order_acquire);
    ShmNode* tail_node = node_at_offset(tail);
    link_node(node, tail_node, nullptr);
    
    header_->size.fetch_add(1, std::memory_order_acq_rel);
    header_->modified_at = current_timestamp_ns();
    stats_.size.fetch_add(1, std::memory_order_relaxed);
    stats_.write_count.fetch_add(1, std::memory_order_relaxed);
    
    return true;
}

bool FastList::add(size_t index, const uint8_t* data, size_t size, int32_t ttl_seconds) {
    if (!data || size == 0) return false;
    
    IpcScopedLock lock(header_->global_mutex);
    
    size_t current_size = header_->size.load(std::memory_order_acquire);
    
    if (index > current_size) {
        return false;
    }
    
    if (index == current_size) {
        // Add at end - unlock and call regular add
        lock.unlock();
        return add(data, size, ttl_seconds);
    }
    
    if (index == 0) {
        // Add at front
        lock.unlock();
        return addFirst(data, size, ttl_seconds);
    }
    
    // Insert in middle
    ShmNode* next_node = node_at_index(index);
    if (!next_node) return false;
    
    ShmNode* prev_node = node_at_offset(next_node->prev_offset.load(std::memory_order_acquire));
    
    ShmNode* node = allocate_node(size);
    SerializationUtil::copy_to_node(node, data, size, ttl_seconds);
    link_node(node, prev_node, next_node);
    
    header_->size.fetch_add(1, std::memory_order_acq_rel);
    header_->modified_at = current_timestamp_ns();
    stats_.size.fetch_add(1, std::memory_order_relaxed);
    stats_.write_count.fetch_add(1, std::memory_order_relaxed);
    
    return true;
}

bool FastList::addFirst(const uint8_t* data, size_t size, int32_t ttl_seconds) {
    if (!data || size == 0) return false;
    
    IpcScopedLock lock(header_->global_mutex);
    
    ShmNode* node = allocate_node(size);
    SerializationUtil::copy_to_node(node, data, size, ttl_seconds);
    
    int64_t head = header_->head_offset.load(std::memory_order_acquire);
    ShmNode* head_node = node_at_offset(head);
    link_node(node, nullptr, head_node);
    
    header_->size.fetch_add(1, std::memory_order_acq_rel);
    header_->modified_at = current_timestamp_ns();
    stats_.size.fetch_add(1, std::memory_order_relaxed);
    stats_.write_count.fetch_add(1, std::memory_order_relaxed);
    
    return true;
}

bool FastList::get(size_t index, std::vector<uint8_t>& out_data) const {
    IpcScopedLock lock(const_cast<IpcSharedMutex&>(header_->global_mutex));
    
    ShmNode* node = node_at_index(index);
    if (!node || !node->entry.is_alive()) {
        const_cast<CollectionStats&>(stats_).miss_count.fetch_add(1, std::memory_order_relaxed);
        return false;
    }
    
    out_data = SerializationUtil::copy_from_node(node);
    const_cast<CollectionStats&>(stats_).read_count.fetch_add(1, std::memory_order_relaxed);
    const_cast<CollectionStats&>(stats_).hit_count.fetch_add(1, std::memory_order_relaxed);
    
    return true;
}

bool FastList::getFirst(std::vector<uint8_t>& out_data) const {
    IpcScopedLock lock(const_cast<IpcSharedMutex&>(header_->global_mutex));
    
    int64_t head = header_->head_offset.load(std::memory_order_acquire);
    ShmNode* node = node_at_offset(head);
    
    // Skip expired nodes
    while (node && node->entry.is_expired()) {
        head = node->next_offset.load(std::memory_order_acquire);
        node = node_at_offset(head);
    }
    
    if (!node || !node->entry.is_alive()) {
        const_cast<CollectionStats&>(stats_).miss_count.fetch_add(1, std::memory_order_relaxed);
        return false;
    }
    
    out_data = SerializationUtil::copy_from_node(node);
    const_cast<CollectionStats&>(stats_).read_count.fetch_add(1, std::memory_order_relaxed);
    const_cast<CollectionStats&>(stats_).hit_count.fetch_add(1, std::memory_order_relaxed);
    
    return true;
}

bool FastList::getLast(std::vector<uint8_t>& out_data) const {
    IpcScopedLock lock(const_cast<IpcSharedMutex&>(header_->global_mutex));
    
    int64_t tail = header_->tail_offset.load(std::memory_order_acquire);
    ShmNode* node = node_at_offset(tail);
    
    // Skip expired nodes
    while (node && node->entry.is_expired()) {
        tail = node->prev_offset.load(std::memory_order_acquire);
        node = node_at_offset(tail);
    }
    
    if (!node || !node->entry.is_alive()) {
        const_cast<CollectionStats&>(stats_).miss_count.fetch_add(1, std::memory_order_relaxed);
        return false;
    }
    
    out_data = SerializationUtil::copy_from_node(node);
    const_cast<CollectionStats&>(stats_).read_count.fetch_add(1, std::memory_order_relaxed);
    const_cast<CollectionStats&>(stats_).hit_count.fetch_add(1, std::memory_order_relaxed);
    
    return true;
}

int64_t FastList::getTTL(size_t index) const {
    IpcScopedLock lock(const_cast<IpcSharedMutex&>(header_->global_mutex));
    
    ShmNode* node = node_at_index(index);
    if (!node) return 0;
    
    return node->entry.remaining_ttl_seconds();
}

bool FastList::set(size_t index, const uint8_t* data, size_t size, int32_t ttl_seconds) {
    if (!data || size == 0) return false;
    
    IpcScopedLock lock(header_->global_mutex);
    
    ShmNode* node = node_at_index(index);
    if (!node) return false;
    
    // If size matches, update in place
    if (node->entry.data_size == size) {
        std::memcpy(node->data, data, size);
        node->entry.hash_code = compute_hash(data, size);
        node->entry.set_ttl(ttl_seconds);
        node->entry.mark_valid();
    } else {
        // Need to reallocate - remove and add
        void* base = file_manager_->segment_manager();
        int64_t prev = node->prev_offset.load(std::memory_order_acquire);
        int64_t next = node->next_offset.load(std::memory_order_acquire);
        
        ShmNode* new_node = allocate_node(size);
        SerializationUtil::copy_to_node(new_node, data, size, ttl_seconds);
        
        // Link new node
        new_node->prev_offset.store(prev, std::memory_order_release);
        new_node->next_offset.store(next, std::memory_order_release);
        
        int64_t new_offset = static_cast<uint8_t*>(static_cast<void*>(new_node)) - 
                             static_cast<uint8_t*>(base);
        
        if (prev >= 0) {
            ShmNode* prev_node = node_at_offset(prev);
            prev_node->next_offset.store(new_offset, std::memory_order_release);
        } else {
            header_->head_offset.store(new_offset, std::memory_order_release);
        }
        
        if (next >= 0) {
            ShmNode* next_node = node_at_offset(next);
            next_node->prev_offset.store(new_offset, std::memory_order_release);
        } else {
            header_->tail_offset.store(new_offset, std::memory_order_release);
        }
        
        // Free old node
        free_node(node, node->entry.data_size);
    }
    
    header_->modified_at = current_timestamp_ns();
    stats_.write_count.fetch_add(1, std::memory_order_relaxed);
    
    // Invalidate cache
    access_cache_.last_index = SIZE_MAX;
    access_cache_.last_offset = -1;
    
    return true;
}

bool FastList::setTTL(size_t index, int32_t ttl_seconds) {
    IpcScopedLock lock(header_->global_mutex);
    
    ShmNode* node = node_at_index(index);
    if (!node || !node->entry.is_alive()) return false;
    
    node->entry.set_ttl(ttl_seconds);
    header_->modified_at = current_timestamp_ns();
    
    return true;
}

bool FastList::remove(size_t index, std::vector<uint8_t>* out_data) {
    IpcScopedLock lock(header_->global_mutex);
    
    ShmNode* node = node_at_index(index);
    if (!node) return false;
    
    if (out_data && node->entry.is_alive()) {
        *out_data = SerializationUtil::copy_from_node(node);
    }
    
    size_t data_size = node->entry.data_size;
    unlink_node(node);
    node->entry.mark_deleted();
    free_node(node, data_size);
    
    header_->size.fetch_sub(1, std::memory_order_acq_rel);
    header_->modified_at = current_timestamp_ns();
    stats_.size.fetch_sub(1, std::memory_order_relaxed);
    
    return true;
}

bool FastList::removeFirst(std::vector<uint8_t>* out_data) {
    IpcScopedLock lock(header_->global_mutex);
    
    int64_t head = header_->head_offset.load(std::memory_order_acquire);
    ShmNode* node = node_at_offset(head);
    
    if (!node) return false;
    
    if (out_data && node->entry.is_alive()) {
        *out_data = SerializationUtil::copy_from_node(node);
    }
    
    size_t data_size = node->entry.data_size;
    unlink_node(node);
    node->entry.mark_deleted();
    free_node(node, data_size);
    
    header_->size.fetch_sub(1, std::memory_order_acq_rel);
    header_->modified_at = current_timestamp_ns();
    stats_.size.fetch_sub(1, std::memory_order_relaxed);
    
    return true;
}

bool FastList::removeLast(std::vector<uint8_t>* out_data) {
    IpcScopedLock lock(header_->global_mutex);
    
    int64_t tail = header_->tail_offset.load(std::memory_order_acquire);
    ShmNode* node = node_at_offset(tail);
    
    if (!node) return false;
    
    if (out_data && node->entry.is_alive()) {
        *out_data = SerializationUtil::copy_from_node(node);
    }
    
    size_t data_size = node->entry.data_size;
    unlink_node(node);
    node->entry.mark_deleted();
    free_node(node, data_size);
    
    header_->size.fetch_sub(1, std::memory_order_acq_rel);
    header_->modified_at = current_timestamp_ns();
    stats_.size.fetch_sub(1, std::memory_order_relaxed);
    
    return true;
}

bool FastList::removeElement(const uint8_t* data, size_t size) {
    if (!data || size == 0) return false;
    
    uint32_t target_hash = compute_hash(data, size);
    
    IpcScopedLock lock(header_->global_mutex);
    
    int64_t current = header_->head_offset.load(std::memory_order_acquire);
    
    while (current >= 0) {
        ShmNode* node = node_at_offset(current);
        if (!node) break;
        
        if (node->entry.is_alive() &&
            node->entry.hash_code == target_hash &&
            node->entry.data_size == size &&
            std::memcmp(node->data, data, size) == 0) {
            
            size_t data_size = node->entry.data_size;
            unlink_node(node);
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

size_t FastList::removeExpired() {
    IpcScopedLock lock(header_->global_mutex);
    
    size_t removed = 0;
    int64_t current = header_->head_offset.load(std::memory_order_acquire);
    
    while (current >= 0) {
        ShmNode* node = node_at_offset(current);
        if (!node) break;
        
        int64_t next = node->next_offset.load(std::memory_order_acquire);
        
        if (node->entry.is_expired()) {
            size_t data_size = node->entry.data_size;
            unlink_node(node);
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

bool FastList::contains(const uint8_t* data, size_t size) const {
    return indexOf(data, size) >= 0;
}

int64_t FastList::indexOf(const uint8_t* data, size_t size) const {
    if (!data || size == 0) return -1;
    
    uint32_t target_hash = compute_hash(data, size);
    
    IpcScopedLock lock(const_cast<IpcSharedMutex&>(header_->global_mutex));
    
    int64_t current = header_->head_offset.load(std::memory_order_acquire);
    int64_t index = 0;
    
    while (current >= 0) {
        ShmNode* node = node_at_offset(current);
        if (!node) break;
        
        if (node->entry.is_alive()) {
            if (node->entry.hash_code == target_hash &&
                node->entry.data_size == size &&
                std::memcmp(node->data, data, size) == 0) {
                return index;
            }
            index++;
        }
        
        current = node->next_offset.load(std::memory_order_acquire);
    }
    
    return -1;
}

int64_t FastList::lastIndexOf(const uint8_t* data, size_t size) const {
    if (!data || size == 0) return -1;
    
    uint32_t target_hash = compute_hash(data, size);
    
    IpcScopedLock lock(const_cast<IpcSharedMutex&>(header_->global_mutex));
    
    // First, count total alive nodes
    int64_t current = header_->head_offset.load(std::memory_order_acquire);
    int64_t total_alive = 0;
    
    while (current >= 0) {
        ShmNode* node = node_at_offset(current);
        if (!node) break;
        if (node->entry.is_alive()) total_alive++;
        current = node->next_offset.load(std::memory_order_acquire);
    }
    
    // Search from tail
    int64_t tail = header_->tail_offset.load(std::memory_order_acquire);
    current = tail;
    int64_t live_index = total_alive - 1;
    
    while (current >= 0) {
        ShmNode* node = node_at_offset(current);
        if (!node) break;
        
        if (node->entry.is_alive()) {
            if (node->entry.hash_code == target_hash &&
                node->entry.data_size == size &&
                std::memcmp(node->data, data, size) == 0) {
                return live_index;
            }
            live_index--;
        }
        
        current = node->prev_offset.load(std::memory_order_acquire);
    }
    
    return -1;
}

void FastList::clear() {
    IpcScopedLock lock(header_->global_mutex);
    
    int64_t current = header_->head_offset.load(std::memory_order_acquire);
    
    while (current >= 0) {
        ShmNode* node = node_at_offset(current);
        if (!node) break;
        
        int64_t next = node->next_offset.load(std::memory_order_acquire);
        size_t data_size = node->entry.data_size;
        node->entry.mark_deleted();
        free_node(node, data_size);
        
        current = next;
    }
    
    header_->head_offset.store(ShmNode::NULL_OFFSET, std::memory_order_release);
    header_->tail_offset.store(ShmNode::NULL_OFFSET, std::memory_order_release);
    header_->size.store(0, std::memory_order_release);
    header_->modified_at = current_timestamp_ns();
    
    stats_.size.store(0, std::memory_order_relaxed);
    access_cache_.last_index = SIZE_MAX;
    access_cache_.last_offset = -1;
}

size_t FastList::size() const {
    // Count only non-expired elements
    IpcScopedLock lock(const_cast<IpcSharedMutex&>(header_->global_mutex));
    
    size_t alive_count = 0;
    int64_t current = header_->head_offset.load(std::memory_order_acquire);
    
    while (current >= 0) {
        ShmNode* node = node_at_offset(current);
        if (!node) break;
        if (node->entry.is_alive()) alive_count++;
        current = node->next_offset.load(std::memory_order_acquire);
    }
    
    return alive_count;
}

bool FastList::isEmpty() const {
    return size() == 0;
}

void FastList::forEach(std::function<bool(const uint8_t* data, size_t size, size_t index)> callback) const {
    IpcScopedLock lock(const_cast<IpcSharedMutex&>(header_->global_mutex));
    
    int64_t current = header_->head_offset.load(std::memory_order_acquire);
    size_t index = 0;
    
    while (current >= 0) {
        ShmNode* node = node_at_offset(current);
        if (!node) break;
        
        if (node->entry.is_alive()) {
            if (!callback(node->data, node->entry.data_size, index)) {
                break;
            }
            index++;
        }
        
        current = node->next_offset.load(std::memory_order_acquire);
    }
}

void FastList::forEachWithTTL(std::function<bool(const uint8_t* data, size_t size, 
                                                  size_t index, int64_t ttl_remaining)> callback) const {
    IpcScopedLock lock(const_cast<IpcSharedMutex&>(header_->global_mutex));
    
    int64_t current = header_->head_offset.load(std::memory_order_acquire);
    size_t index = 0;
    
    while (current >= 0) {
        ShmNode* node = node_at_offset(current);
        if (!node) break;
        
        if (node->entry.is_alive()) {
            int64_t ttl_remaining = node->entry.remaining_ttl_seconds();
            if (!callback(node->data, node->entry.data_size, index, ttl_remaining)) {
                break;
            }
            index++;
        }
        
        current = node->next_offset.load(std::memory_order_acquire);
    }
}

void FastList::flush() {
    file_manager_->flush();
}

void FastList::lazy_cleanup_expired() const {
    // Called internally to clean up a limited number of expired nodes
    // This is const because it's logically const (doesn't change visible state)
    // but physically modifies the structure
    const_cast<FastList*>(this)->removeExpired();
}

} // namespace fastcollection
