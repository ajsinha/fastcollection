/**
 * Copyright © 2025-2030, All Rights Reserved
 * Ashutosh Sinha | Email: ajsinha@gmail.com
 * 
 * Patent Pending
 * 
 * @file fc_stack.cpp
 * @brief Implementation of high-performance memory-mapped stack with TTL
 */

#include "fc_stack.h"
#include <cstring>

namespace fastcollection {

using IpcScopedLock = bip::scoped_lock<IpcSharedMutex>;

FastStack::FastStack(const std::string& mmap_file,
                     size_t initial_size,
                     bool create_new)
    : file_manager_(std::make_unique<MMapFileManager>(mmap_file, initial_size, create_new)) {
    
    auto result = file_manager_->find<DequeHeader>("stack_header");
    
    if (result.first) {
        header_ = result.first;
        if (!header_->is_valid()) {
            throw FastCollectionException(
                FastCollectionException::ErrorCode::INTERNAL_ERROR,
                "Invalid stack header in file"
            );
        }
    } else {
        header_ = file_manager_->find_or_construct<DequeHeader>("stack_header")();
    }
    
    // Find or create ABA counter
    auto aba_result = file_manager_->find<std::atomic<uint64_t>>("stack_aba_tag");
    if (aba_result.first) {
        aba_tag_ = aba_result.first;
    } else {
        aba_tag_ = file_manager_->find_or_construct<std::atomic<uint64_t>>("stack_aba_tag")(0);
    }
    
    stats_.size.store(header_->size.load(), std::memory_order_relaxed);
}

FastStack::~FastStack() {
    if (file_manager_) {
        flush();
    }
}

FastStack::FastStack(FastStack&& other) noexcept
    : file_manager_(std::move(other.file_manager_))
    , header_(other.header_)
    , aba_tag_(other.aba_tag_) {
    other.header_ = nullptr;
    other.aba_tag_ = nullptr;
}

FastStack& FastStack::operator=(FastStack&& other) noexcept {
    if (this != &other) {
        file_manager_ = std::move(other.file_manager_);
        header_ = other.header_;
        aba_tag_ = other.aba_tag_;
        other.header_ = nullptr;
        other.aba_tag_ = nullptr;
    }
    return *this;
}

ShmNode* FastStack::node_at_offset(int64_t offset) const {
    if (offset < 0) return nullptr;
    
    void* base = file_manager_->segment_manager();
    return reinterpret_cast<ShmNode*>(static_cast<uint8_t*>(base) + offset);
}

ShmNode* FastStack::allocate_node(size_t data_size) {
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

void FastStack::free_node(ShmNode* node, size_t data_size) {
    if (node) {
        file_manager_->deallocate(node);
    }
}

bool FastStack::push(const uint8_t* data, size_t size, int32_t ttl_seconds) {
    if (!data || size == 0) return false;
    
    void* base = file_manager_->segment_manager();
    
    ShmNode* node = allocate_node(size);
    SerializationUtil::copy_to_node(node, data, size, ttl_seconds);
    
    int64_t node_offset = static_cast<uint8_t*>(static_cast<void*>(node)) - 
                          static_cast<uint8_t*>(base);
    
    // CAS loop for lock-free push
    while (true) {
        int64_t old_top = header_->front_offset.load(std::memory_order_acquire);
        node->next_offset.store(old_top, std::memory_order_release);
        node->prev_offset.store(ShmNode::NULL_OFFSET, std::memory_order_release);
        
        if (header_->front_offset.compare_exchange_weak(
                old_top, node_offset,
                std::memory_order_acq_rel,
                std::memory_order_relaxed)) {
            
            // Update previous top's prev pointer (for iteration)
            if (old_top >= 0) {
                ShmNode* old_top_node = node_at_offset(old_top);
                old_top_node->prev_offset.store(node_offset, std::memory_order_release);
            }
            
            // Increment ABA tag
            aba_tag_->fetch_add(1, std::memory_order_relaxed);
            header_->size.fetch_add(1, std::memory_order_acq_rel);
            header_->modified_at = current_timestamp_ns();
            
            stats_.size.fetch_add(1, std::memory_order_relaxed);
            stats_.write_count.fetch_add(1, std::memory_order_relaxed);
            
            return true;
        }
        // CAS failed, retry
    }
}

bool FastStack::pop(std::vector<uint8_t>& out_data) {
    while (true) {
        int64_t top = header_->front_offset.load(std::memory_order_acquire);
        
        if (top < 0) {
            stats_.miss_count.fetch_add(1, std::memory_order_relaxed);
            return false;
        }
        
        ShmNode* node = node_at_offset(top);
        if (!node) {
            stats_.miss_count.fetch_add(1, std::memory_order_relaxed);
            return false;
        }
        
        // Skip expired nodes
        if (node->entry.is_expired()) {
            // Try to remove expired node
            int64_t next = node->next_offset.load(std::memory_order_acquire);
            
            if (header_->front_offset.compare_exchange_weak(
                    top, next,
                    std::memory_order_acq_rel,
                    std::memory_order_relaxed)) {
                
                // Successfully removed expired node
                if (next >= 0) {
                    ShmNode* next_node = node_at_offset(next);
                    next_node->prev_offset.store(ShmNode::NULL_OFFSET, std::memory_order_release);
                }
                
                size_t data_size = node->entry.data_size;
                node->entry.mark_deleted();
                free_node(node, data_size);
                
                aba_tag_->fetch_add(1, std::memory_order_relaxed);
                header_->size.fetch_sub(1, std::memory_order_acq_rel);
                stats_.size.fetch_sub(1, std::memory_order_relaxed);
            }
            // Retry from the beginning
            continue;
        }
        
        // Found valid non-expired node
        int64_t next = node->next_offset.load(std::memory_order_acquire);
        
        // Copy data before CAS (in case another thread pops it)
        std::vector<uint8_t> temp_data = SerializationUtil::copy_from_node(node);
        
        if (header_->front_offset.compare_exchange_weak(
                top, next,
                std::memory_order_acq_rel,
                std::memory_order_relaxed)) {
            
            // Successfully popped
            if (next >= 0) {
                ShmNode* next_node = node_at_offset(next);
                next_node->prev_offset.store(ShmNode::NULL_OFFSET, std::memory_order_release);
            }
            
            out_data = std::move(temp_data);
            
            size_t data_size = node->entry.data_size;
            node->entry.mark_deleted();
            free_node(node, data_size);
            
            aba_tag_->fetch_add(1, std::memory_order_relaxed);
            header_->size.fetch_sub(1, std::memory_order_acq_rel);
            header_->modified_at = current_timestamp_ns();
            
            stats_.size.fetch_sub(1, std::memory_order_relaxed);
            stats_.read_count.fetch_add(1, std::memory_order_relaxed);
            stats_.hit_count.fetch_add(1, std::memory_order_relaxed);
            
            return true;
        }
        // CAS failed, retry
    }
}

bool FastStack::peek(std::vector<uint8_t>& out_data) const {
    int64_t top = header_->front_offset.load(std::memory_order_acquire);
    
    while (top >= 0) {
        ShmNode* node = node_at_offset(top);
        if (!node) break;
        
        if (node->entry.is_alive()) {
            out_data = SerializationUtil::copy_from_node(node);
            const_cast<CollectionStats&>(stats_).read_count.fetch_add(1, std::memory_order_relaxed);
            const_cast<CollectionStats&>(stats_).hit_count.fetch_add(1, std::memory_order_relaxed);
            return true;
        }
        
        // Skip expired
        top = node->next_offset.load(std::memory_order_acquire);
    }
    
    const_cast<CollectionStats&>(stats_).miss_count.fetch_add(1, std::memory_order_relaxed);
    return false;
}

std::vector<uint8_t> FastStack::popOrThrow() {
    std::vector<uint8_t> result;
    if (!pop(result)) {
        throw FastCollectionException(
            FastCollectionException::ErrorCode::NOT_FOUND,
            "Stack is empty"
        );
    }
    return result;
}

std::vector<uint8_t> FastStack::peekOrThrow() const {
    std::vector<uint8_t> result;
    if (!peek(result)) {
        throw FastCollectionException(
            FastCollectionException::ErrorCode::NOT_FOUND,
            "Stack is empty"
        );
    }
    return result;
}

size_t FastStack::pushAll(const std::vector<std::tuple<const uint8_t*, size_t, int32_t>>& elements) {
    size_t pushed = 0;
    for (const auto& [data, size, ttl] : elements) {
        if (push(data, size, ttl)) {
            pushed++;
        }
    }
    return pushed;
}

size_t FastStack::popAll(std::vector<std::vector<uint8_t>>& out_data, size_t max_count) {
    size_t popped = 0;
    std::vector<uint8_t> item;
    
    while (popped < max_count && pop(item)) {
        out_data.push_back(std::move(item));
        popped++;
    }
    
    return popped;
}

int64_t FastStack::peekTTL() const {
    int64_t top = header_->front_offset.load(std::memory_order_acquire);
    
    while (top >= 0) {
        ShmNode* node = node_at_offset(top);
        if (!node) break;
        
        if (node->entry.is_alive()) {
            return node->entry.remaining_ttl_seconds();
        }
        
        top = node->next_offset.load(std::memory_order_acquire);
    }
    
    return 0;
}

size_t FastStack::removeExpired() {
    // Use locking for bulk removal
    IpcScopedLock lock(header_->global_mutex);
    
    size_t removed = 0;
    void* base = file_manager_->segment_manager();
    
    int64_t current = header_->front_offset.load(std::memory_order_acquire);
    int64_t prev_offset = ShmNode::NULL_OFFSET;
    
    while (current >= 0) {
        ShmNode* node = node_at_offset(current);
        if (!node) break;
        
        int64_t next = node->next_offset.load(std::memory_order_acquire);
        
        if (node->entry.is_expired()) {
            // Unlink node
            if (prev_offset < 0) {
                header_->front_offset.store(next, std::memory_order_release);
            } else {
                ShmNode* prev_node = node_at_offset(prev_offset);
                prev_node->next_offset.store(next, std::memory_order_release);
            }
            
            if (next >= 0) {
                ShmNode* next_node = node_at_offset(next);
                next_node->prev_offset.store(prev_offset, std::memory_order_release);
            }
            
            size_t data_size = node->entry.data_size;
            node->entry.mark_deleted();
            free_node(node, data_size);
            
            header_->size.fetch_sub(1, std::memory_order_acq_rel);
            stats_.size.fetch_sub(1, std::memory_order_relaxed);
            removed++;
        } else {
            prev_offset = current;
        }
        
        current = next;
    }
    
    if (removed > 0) {
        aba_tag_->fetch_add(1, std::memory_order_relaxed);
        header_->modified_at = current_timestamp_ns();
    }
    
    return removed;
}

int64_t FastStack::search(const uint8_t* data, size_t size) const {
    if (!data || size == 0) return -1;
    
    uint32_t hash = compute_hash(data, size);
    int64_t top = header_->front_offset.load(std::memory_order_acquire);
    int64_t distance = 1;  // 1-based distance
    
    while (top >= 0) {
        ShmNode* node = node_at_offset(top);
        if (!node) break;
        
        if (node->entry.is_alive()) {
            if (node->entry.hash_code == hash &&
                node->entry.data_size == size &&
                std::memcmp(node->data, data, size) == 0) {
                return distance;
            }
            distance++;
        }
        
        top = node->next_offset.load(std::memory_order_acquire);
    }
    
    return -1;
}

bool FastStack::removeElement(const uint8_t* data, size_t size) {
    if (!data || size == 0) return false;
    
    uint32_t hash = compute_hash(data, size);
    
    // Use locking for removal from middle
    IpcScopedLock lock(header_->global_mutex);
    
    void* base = file_manager_->segment_manager();
    int64_t current = header_->front_offset.load(std::memory_order_acquire);
    int64_t prev_offset = ShmNode::NULL_OFFSET;
    
    while (current >= 0) {
        ShmNode* node = node_at_offset(current);
        if (!node) break;
        
        if (node->entry.is_alive() &&
            node->entry.hash_code == hash &&
            node->entry.data_size == size &&
            std::memcmp(node->data, data, size) == 0) {
            
            int64_t next = node->next_offset.load(std::memory_order_acquire);
            
            if (prev_offset < 0) {
                header_->front_offset.store(next, std::memory_order_release);
            } else {
                ShmNode* prev_node = node_at_offset(prev_offset);
                prev_node->next_offset.store(next, std::memory_order_release);
            }
            
            if (next >= 0) {
                ShmNode* next_node = node_at_offset(next);
                next_node->prev_offset.store(prev_offset, std::memory_order_release);
            }
            
            size_t data_size = node->entry.data_size;
            node->entry.mark_deleted();
            free_node(node, data_size);
            
            aba_tag_->fetch_add(1, std::memory_order_relaxed);
            header_->size.fetch_sub(1, std::memory_order_acq_rel);
            header_->modified_at = current_timestamp_ns();
            stats_.size.fetch_sub(1, std::memory_order_relaxed);
            
            return true;
        }
        
        prev_offset = current;
        current = node->next_offset.load(std::memory_order_acquire);
    }
    
    return false;
}

void FastStack::clear() {
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
    header_->size.store(0, std::memory_order_release);
    header_->modified_at = current_timestamp_ns();
    
    aba_tag_->fetch_add(1, std::memory_order_relaxed);
    stats_.size.store(0, std::memory_order_relaxed);
}

size_t FastStack::size() const {
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

bool FastStack::isEmpty() const {
    return size() == 0;
}

void FastStack::forEach(std::function<bool(const uint8_t* data, size_t size)> callback) const {
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

void FastStack::forEachWithTTL(std::function<bool(const uint8_t* data, size_t size,
                                                   int64_t ttl_remaining)> callback) const {
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

void FastStack::flush() {
    file_manager_->flush();
}

} // namespace fastcollection
