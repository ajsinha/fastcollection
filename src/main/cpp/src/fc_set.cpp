/**
 * Copyright © 2025-2030, All Rights Reserved
 * Ashutosh Sinha | Email: ajsinha@gmail.com
 * 
 * Patent Pending
 * 
 * @file fc_set.cpp
 * @brief Implementation of high-performance memory-mapped set with TTL
 */

#include "fc_set.h"
#include <cstring>

namespace fastcollection {

using IpcScopedLock = bip::scoped_lock<IpcMutex>;

FastSet::FastSet(const std::string& mmap_file,
                 size_t initial_size,
                 bool create_new,
                 uint32_t bucket_count)
    : file_manager_(std::make_unique<MMapFileManager>(mmap_file, initial_size, create_new)) {
    
    auto result = file_manager_->find<HashTableHeader>("set_header");
    
    if (result.first) {
        header_ = result.first;
        if (!header_->is_valid()) {
            throw FastCollectionException(
                FastCollectionException::ErrorCode::INTERNAL_ERROR,
                "Invalid set header in file"
            );
        }
    } else {
        header_ = file_manager_->find_or_construct<HashTableHeader>("set_header", bucket_count);
    }
    
    // Find or create buckets
    auto buckets_result = file_manager_->find<ShmBucket>("set_buckets");
    if (buckets_result.first) {
        buckets_ = buckets_result.first;
    } else {
        buckets_ = file_manager_->construct_array<ShmBucket>("set_buckets", header_->bucket_count);
    }
    
    stats_.size.store(header_->size.load(), std::memory_order_relaxed);
}

FastSet::~FastSet() {
    if (file_manager_) {
        flush();
    }
}

FastSet::FastSet(FastSet&& other) noexcept
    : file_manager_(std::move(other.file_manager_))
    , header_(other.header_)
    , buckets_(other.buckets_) {
    other.header_ = nullptr;
    other.buckets_ = nullptr;
}

FastSet& FastSet::operator=(FastSet&& other) noexcept {
    if (this != &other) {
        file_manager_ = std::move(other.file_manager_);
        header_ = other.header_;
        buckets_ = other.buckets_;
        other.header_ = nullptr;
        other.buckets_ = nullptr;
    }
    return *this;
}

ShmBucket* FastSet::get_bucket(uint32_t hash) {
    uint32_t idx = hash & (header_->bucket_count - 1);
    return &buckets_[idx];
}

const ShmBucket* FastSet::get_bucket(uint32_t hash) const {
    uint32_t idx = hash & (header_->bucket_count - 1);
    return &buckets_[idx];
}

ShmNode* FastSet::find_in_bucket(ShmBucket* bucket, const uint8_t* data, size_t size,
                                  uint32_t hash, ShmNode** prev_out) {
    void* base = file_manager_->segment_manager();
    
    int64_t current = bucket->head_offset.load(std::memory_order_acquire);
    ShmNode* prev = nullptr;
    
    while (current >= 0) {
        ShmNode* node = reinterpret_cast<ShmNode*>(
            static_cast<uint8_t*>(base) + current
        );
        
        if (node->entry.is_alive() &&
            node->entry.hash_code == hash &&
            node->entry.data_size == size &&
            std::memcmp(node->data, data, size) == 0) {
            if (prev_out) *prev_out = prev;
            return node;
        }
        
        prev = node;
        current = node->next_offset.load(std::memory_order_acquire);
    }
    
    if (prev_out) *prev_out = prev;
    return nullptr;
}

ShmNode* FastSet::allocate_node(size_t data_size) {
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

void FastSet::free_node(ShmNode* node) {
    if (node) {
        file_manager_->deallocate(node);
    }
}

bool FastSet::add(const uint8_t* data, size_t size, int32_t ttl_seconds) {
    if (!data || size == 0) return false;
    
    uint32_t hash = compute_hash(data, size);
    ShmBucket* bucket = get_bucket(hash);
    
    IpcScopedLock lock(bucket->mutex);
    
    // Check if already exists
    ShmNode* existing = find_in_bucket(bucket, data, size, hash, nullptr);
    if (existing) {
        if (existing->entry.is_alive()) {
            // Already exists and not expired
            return false;
        }
        // Expired - update in place
        existing->entry.set_ttl(ttl_seconds);
        existing->entry.mark_valid();
        stats_.write_count.fetch_add(1, std::memory_order_relaxed);
        return true;
    }
    
    // Allocate and add new node
    void* base = file_manager_->segment_manager();
    ShmNode* node = allocate_node(size);
    SerializationUtil::copy_to_node(node, data, size, ttl_seconds);
    
    int64_t node_offset = static_cast<uint8_t*>(static_cast<void*>(node)) - 
                          static_cast<uint8_t*>(base);
    
    // Link at head of bucket chain
    int64_t old_head = bucket->head_offset.load(std::memory_order_acquire);
    node->next_offset.store(old_head, std::memory_order_release);
    node->prev_offset.store(ShmNode::NULL_OFFSET, std::memory_order_release);
    
    if (old_head >= 0) {
        ShmNode* old_head_node = reinterpret_cast<ShmNode*>(
            static_cast<uint8_t*>(base) + old_head
        );
        old_head_node->prev_offset.store(node_offset, std::memory_order_release);
    }
    
    bucket->head_offset.store(node_offset, std::memory_order_release);
    bucket->size.fetch_add(1, std::memory_order_acq_rel);
    
    header_->size.fetch_add(1, std::memory_order_acq_rel);
    header_->modified_at = current_timestamp_ns();
    stats_.size.fetch_add(1, std::memory_order_relaxed);
    stats_.write_count.fetch_add(1, std::memory_order_relaxed);
    
    return true;
}

bool FastSet::remove(const uint8_t* data, size_t size) {
    if (!data || size == 0) return false;
    
    uint32_t hash = compute_hash(data, size);
    ShmBucket* bucket = get_bucket(hash);
    
    IpcScopedLock lock(bucket->mutex);
    
    void* base = file_manager_->segment_manager();
    ShmNode* prev = nullptr;
    ShmNode* node = find_in_bucket(bucket, data, size, hash, &prev);
    
    if (!node || !node->entry.is_alive()) {
        return false;
    }
    
    // Unlink from chain
    int64_t next = node->next_offset.load(std::memory_order_acquire);
    int64_t node_offset = static_cast<uint8_t*>(static_cast<void*>(node)) - 
                          static_cast<uint8_t*>(base);
    
    if (prev) {
        prev->next_offset.store(next, std::memory_order_release);
    } else {
        bucket->head_offset.store(next, std::memory_order_release);
    }
    
    if (next >= 0) {
        ShmNode* next_node = reinterpret_cast<ShmNode*>(
            static_cast<uint8_t*>(base) + next
        );
        next_node->prev_offset.store(
            prev ? (static_cast<uint8_t*>(static_cast<void*>(prev)) - 
                   static_cast<uint8_t*>(base)) : ShmNode::NULL_OFFSET,
            std::memory_order_release
        );
    }
    
    node->entry.mark_deleted();
    free_node(node);
    
    bucket->size.fetch_sub(1, std::memory_order_acq_rel);
    header_->size.fetch_sub(1, std::memory_order_acq_rel);
    header_->modified_at = current_timestamp_ns();
    stats_.size.fetch_sub(1, std::memory_order_relaxed);
    
    return true;
}

bool FastSet::contains(const uint8_t* data, size_t size) const {
    if (!data || size == 0) return false;
    
    uint32_t hash = compute_hash(data, size);
    const ShmBucket* bucket = get_bucket(hash);
    
    // Lock-free optimistic read
    void* base = file_manager_->segment_manager();
    int64_t current = bucket->head_offset.load(std::memory_order_acquire);
    
    while (current >= 0) {
        const ShmNode* node = reinterpret_cast<const ShmNode*>(
            static_cast<const uint8_t*>(base) + current
        );
        
        if (node->entry.is_alive() &&
            node->entry.hash_code == hash &&
            node->entry.data_size == size &&
            std::memcmp(node->data, data, size) == 0) {
            const_cast<CollectionStats&>(stats_).hit_count.fetch_add(1, std::memory_order_relaxed);
            const_cast<CollectionStats&>(stats_).read_count.fetch_add(1, std::memory_order_relaxed);
            return true;
        }
        
        current = node->next_offset.load(std::memory_order_acquire);
    }
    
    const_cast<CollectionStats&>(stats_).miss_count.fetch_add(1, std::memory_order_relaxed);
    const_cast<CollectionStats&>(stats_).read_count.fetch_add(1, std::memory_order_relaxed);
    return false;
}

int64_t FastSet::getTTL(const uint8_t* data, size_t size) const {
    if (!data || size == 0) return 0;
    
    uint32_t hash = compute_hash(data, size);
    const ShmBucket* bucket = get_bucket(hash);
    
    void* base = file_manager_->segment_manager();
    int64_t current = bucket->head_offset.load(std::memory_order_acquire);
    
    while (current >= 0) {
        const ShmNode* node = reinterpret_cast<const ShmNode*>(
            static_cast<const uint8_t*>(base) + current
        );
        
        if (node->entry.is_alive() &&
            node->entry.hash_code == hash &&
            node->entry.data_size == size &&
            std::memcmp(node->data, data, size) == 0) {
            return node->entry.remaining_ttl_seconds();
        }
        
        current = node->next_offset.load(std::memory_order_acquire);
    }
    
    return 0;
}

bool FastSet::setTTL(const uint8_t* data, size_t size, int32_t ttl_seconds) {
    if (!data || size == 0) return false;
    
    uint32_t hash = compute_hash(data, size);
    ShmBucket* bucket = get_bucket(hash);
    
    IpcScopedLock lock(bucket->mutex);
    
    ShmNode* node = find_in_bucket(bucket, data, size, hash, nullptr);
    if (!node || !node->entry.is_alive()) {
        return false;
    }
    
    node->entry.set_ttl(ttl_seconds);
    header_->modified_at = current_timestamp_ns();
    
    return true;
}

size_t FastSet::addAll(const std::vector<std::tuple<const uint8_t*, size_t, int32_t>>& elements) {
    size_t added = 0;
    for (const auto& [data, size, ttl] : elements) {
        if (add(data, size, ttl)) {
            added++;
        }
    }
    return added;
}

size_t FastSet::removeAll(const std::vector<std::pair<const uint8_t*, size_t>>& elements) {
    size_t removed = 0;
    for (const auto& [data, size] : elements) {
        if (remove(data, size)) {
            removed++;
        }
    }
    return removed;
}

size_t FastSet::retainIf(std::function<bool(const uint8_t* data, size_t size)> predicate) {
    size_t removed = 0;
    void* base = file_manager_->segment_manager();
    
    for (uint32_t i = 0; i < header_->bucket_count; i++) {
        ShmBucket* bucket = &buckets_[i];
        IpcScopedLock lock(bucket->mutex);
        
        int64_t current = bucket->head_offset.load(std::memory_order_acquire);
        
        while (current >= 0) {
            ShmNode* node = reinterpret_cast<ShmNode*>(
                static_cast<uint8_t*>(base) + current
            );
            int64_t next = node->next_offset.load(std::memory_order_acquire);
            
            if (node->entry.is_alive() && !predicate(node->data, node->entry.data_size)) {
                // Remove this node
                int64_t prev = node->prev_offset.load(std::memory_order_acquire);
                
                if (prev >= 0) {
                    ShmNode* prev_node = reinterpret_cast<ShmNode*>(
                        static_cast<uint8_t*>(base) + prev
                    );
                    prev_node->next_offset.store(next, std::memory_order_release);
                } else {
                    bucket->head_offset.store(next, std::memory_order_release);
                }
                
                if (next >= 0) {
                    ShmNode* next_node = reinterpret_cast<ShmNode*>(
                        static_cast<uint8_t*>(base) + next
                    );
                    next_node->prev_offset.store(prev, std::memory_order_release);
                }
                
                node->entry.mark_deleted();
                free_node(node);
                
                bucket->size.fetch_sub(1, std::memory_order_acq_rel);
                header_->size.fetch_sub(1, std::memory_order_acq_rel);
                stats_.size.fetch_sub(1, std::memory_order_relaxed);
                removed++;
            }
            
            current = next;
        }
    }
    
    if (removed > 0) {
        header_->modified_at = current_timestamp_ns();
    }
    
    return removed;
}

size_t FastSet::removeExpired() {
    size_t removed = 0;
    void* base = file_manager_->segment_manager();
    
    for (uint32_t i = 0; i < header_->bucket_count; i++) {
        ShmBucket* bucket = &buckets_[i];
        IpcScopedLock lock(bucket->mutex);
        
        int64_t current = bucket->head_offset.load(std::memory_order_acquire);
        
        while (current >= 0) {
            ShmNode* node = reinterpret_cast<ShmNode*>(
                static_cast<uint8_t*>(base) + current
            );
            int64_t next = node->next_offset.load(std::memory_order_acquire);
            
            if (node->entry.is_expired()) {
                int64_t prev = node->prev_offset.load(std::memory_order_acquire);
                
                if (prev >= 0) {
                    ShmNode* prev_node = reinterpret_cast<ShmNode*>(
                        static_cast<uint8_t*>(base) + prev
                    );
                    prev_node->next_offset.store(next, std::memory_order_release);
                } else {
                    bucket->head_offset.store(next, std::memory_order_release);
                }
                
                if (next >= 0) {
                    ShmNode* next_node = reinterpret_cast<ShmNode*>(
                        static_cast<uint8_t*>(base) + next
                    );
                    next_node->prev_offset.store(prev, std::memory_order_release);
                }
                
                node->entry.mark_deleted();
                free_node(node);
                
                bucket->size.fetch_sub(1, std::memory_order_acq_rel);
                header_->size.fetch_sub(1, std::memory_order_acq_rel);
                stats_.size.fetch_sub(1, std::memory_order_relaxed);
                removed++;
            }
            
            current = next;
        }
    }
    
    if (removed > 0) {
        header_->modified_at = current_timestamp_ns();
    }
    
    return removed;
}

void FastSet::forEach(std::function<bool(const uint8_t* data, size_t size)> callback) const {
    void* base = file_manager_->segment_manager();
    
    for (uint32_t i = 0; i < header_->bucket_count; i++) {
        const ShmBucket* bucket = &buckets_[i];
        int64_t current = bucket->head_offset.load(std::memory_order_acquire);
        
        while (current >= 0) {
            const ShmNode* node = reinterpret_cast<const ShmNode*>(
                static_cast<const uint8_t*>(base) + current
            );
            
            if (node->entry.is_alive()) {
                if (!callback(node->data, node->entry.data_size)) {
                    return;
                }
            }
            
            current = node->next_offset.load(std::memory_order_acquire);
        }
    }
}

void FastSet::forEachWithTTL(std::function<bool(const uint8_t* data, size_t size,
                                                 int64_t ttl_remaining)> callback) const {
    void* base = file_manager_->segment_manager();
    
    for (uint32_t i = 0; i < header_->bucket_count; i++) {
        const ShmBucket* bucket = &buckets_[i];
        int64_t current = bucket->head_offset.load(std::memory_order_acquire);
        
        while (current >= 0) {
            const ShmNode* node = reinterpret_cast<const ShmNode*>(
                static_cast<const uint8_t*>(base) + current
            );
            
            if (node->entry.is_alive()) {
                int64_t ttl = node->entry.remaining_ttl_seconds();
                if (!callback(node->data, node->entry.data_size, ttl)) {
                    return;
                }
            }
            
            current = node->next_offset.load(std::memory_order_acquire);
        }
    }
}

std::vector<std::vector<uint8_t>> FastSet::toArray() const {
    std::vector<std::vector<uint8_t>> result;
    result.reserve(header_->size.load(std::memory_order_acquire));
    
    forEach([&result](const uint8_t* data, size_t size) {
        result.emplace_back(data, data + size);
        return true;
    });
    
    return result;
}

void FastSet::clear() {
    void* base = file_manager_->segment_manager();
    
    for (uint32_t i = 0; i < header_->bucket_count; i++) {
        ShmBucket* bucket = &buckets_[i];
        IpcScopedLock lock(bucket->mutex);
        
        int64_t current = bucket->head_offset.load(std::memory_order_acquire);
        
        while (current >= 0) {
            ShmNode* node = reinterpret_cast<ShmNode*>(
                static_cast<uint8_t*>(base) + current
            );
            int64_t next = node->next_offset.load(std::memory_order_acquire);
            
            node->entry.mark_deleted();
            free_node(node);
            
            current = next;
        }
        
        bucket->head_offset.store(ShmNode::NULL_OFFSET, std::memory_order_release);
        bucket->size.store(0, std::memory_order_release);
    }
    
    header_->size.store(0, std::memory_order_release);
    header_->modified_at = current_timestamp_ns();
    stats_.size.store(0, std::memory_order_relaxed);
}

size_t FastSet::size() const {
    // Count only alive elements
    size_t alive = 0;
    void* base = file_manager_->segment_manager();
    
    for (uint32_t i = 0; i < header_->bucket_count; i++) {
        const ShmBucket* bucket = &buckets_[i];
        int64_t current = bucket->head_offset.load(std::memory_order_acquire);
        
        while (current >= 0) {
            const ShmNode* node = reinterpret_cast<const ShmNode*>(
                static_cast<const uint8_t*>(base) + current
            );
            if (node->entry.is_alive()) alive++;
            current = node->next_offset.load(std::memory_order_acquire);
        }
    }
    
    return alive;
}

bool FastSet::isEmpty() const {
    return size() == 0;
}

void FastSet::flush() {
    file_manager_->flush();
}

} // namespace fastcollection
