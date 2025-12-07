/**
 * Copyright © 2025-2030, All Rights Reserved
 * Ashutosh Sinha | Email: ajsinha@gmail.com
 * 
 * Patent Pending
 * 
 * @file fc_map.cpp
 * @brief Implementation of high-performance memory-mapped map with TTL
 */

#include "fc_map.h"
#include <cstring>

namespace fastcollection {

using IpcScopedLock = bip::scoped_lock<IpcMutex>;

FastMap::FastMap(const std::string& mmap_file,
                 size_t initial_size,
                 bool create_new,
                 uint32_t bucket_count)
    : file_manager_(std::make_unique<MMapFileManager>(mmap_file, initial_size, create_new)) {
    
    auto result = file_manager_->find<HashTableHeader>("map_header");
    
    if (result.first) {
        header_ = result.first;
        if (!header_->is_valid()) {
            throw FastCollectionException(
                FastCollectionException::ErrorCode::INTERNAL_ERROR,
                "Invalid map header in file"
            );
        }
    } else {
        header_ = file_manager_->find_or_construct<HashTableHeader>("map_header")(bucket_count);
    }
    
    auto buckets_result = file_manager_->find<ShmBucket>("map_buckets");
    if (buckets_result.first) {
        buckets_ = buckets_result.first;
    } else {
        buckets_ = file_manager_->construct_array<ShmBucket>("map_buckets", header_->bucket_count);
    }
    
    stats_.size.store(header_->size.load(), std::memory_order_relaxed);
}

FastMap::~FastMap() {
    if (file_manager_) {
        flush();
    }
}

FastMap::FastMap(FastMap&& other) noexcept
    : file_manager_(std::move(other.file_manager_))
    , header_(other.header_)
    , buckets_(other.buckets_) {
    other.header_ = nullptr;
    other.buckets_ = nullptr;
}

FastMap& FastMap::operator=(FastMap&& other) noexcept {
    if (this != &other) {
        file_manager_ = std::move(other.file_manager_);
        header_ = other.header_;
        buckets_ = other.buckets_;
        other.header_ = nullptr;
        other.buckets_ = nullptr;
    }
    return *this;
}

ShmBucket* FastMap::get_bucket(uint32_t hash) {
    uint32_t idx = hash & (header_->bucket_count - 1);
    return &buckets_[idx];
}

const ShmBucket* FastMap::get_bucket(uint32_t hash) const {
    uint32_t idx = hash & (header_->bucket_count - 1);
    return &buckets_[idx];
}

ShmKeyValue* FastMap::find_in_bucket(ShmBucket* bucket, const uint8_t* key, size_t key_size,
                                      uint32_t hash, ShmKeyValue** prev_out) {
    void* base = file_manager_->segment_manager();
    
    int64_t current = bucket->head_offset.load(std::memory_order_acquire);
    ShmKeyValue* prev = nullptr;
    
    while (current >= 0) {
        ShmKeyValue* kv = reinterpret_cast<ShmKeyValue*>(
            static_cast<uint8_t*>(base) + current
        );
        
        if (kv->entry.hash_code == hash &&
            kv->key_size == key_size &&
            std::memcmp(kv->data, key, key_size) == 0) {
            if (prev_out) *prev_out = prev;
            return kv;
        }
        
        prev = kv;
        current = kv->next_offset.load(std::memory_order_acquire);
    }
    
    if (prev_out) *prev_out = prev;
    return nullptr;
}

ShmKeyValue* FastMap::allocate_kv(size_t key_size, size_t value_size) {
    size_t total = ShmKeyValue::total_size(key_size, value_size);
    void* mem = file_manager_->allocate(total);
    if (!mem) {
        throw FastCollectionException(
            FastCollectionException::ErrorCode::MEMORY_ALLOCATION_FAILED,
            "Failed to allocate key-value"
        );
    }
    return new(mem) ShmKeyValue();
}

void FastMap::free_kv(ShmKeyValue* kv) {
    if (kv) {
        file_manager_->deallocate(kv);
    }
}

bool FastMap::put(const uint8_t* key, size_t key_size,
                  const uint8_t* value, size_t value_size,
                  int32_t ttl_seconds) {
    if (!key || key_size == 0) return false;
    
    uint32_t hash = compute_hash(key, key_size);
    ShmBucket* bucket = get_bucket(hash);
    
    IpcScopedLock lock(bucket->mutex);
    
    void* base = file_manager_->segment_manager();
    ShmKeyValue* existing = find_in_bucket(bucket, key, key_size, hash, nullptr);
    
    if (existing) {
        // Update existing entry
        if (existing->value_size == value_size) {
            // Same size - update in place
            std::memcpy(existing->data + key_size, value, value_size);
            existing->entry.set_ttl(ttl_seconds);
            existing->entry.mark_valid();
        } else {
            // Different size - need to reallocate
            int64_t prev_offset = existing->prev_offset.load(std::memory_order_acquire);
            int64_t next_offset = existing->next_offset.load(std::memory_order_acquire);
            
            ShmKeyValue* new_kv = allocate_kv(key_size, value_size);
            SerializationUtil::copy_to_kv(new_kv, key, key_size, value, value_size, ttl_seconds);
            
            int64_t new_offset = static_cast<uint8_t*>(static_cast<void*>(new_kv)) - 
                                 static_cast<uint8_t*>(base);
            
            new_kv->prev_offset.store(prev_offset, std::memory_order_release);
            new_kv->next_offset.store(next_offset, std::memory_order_release);
            
            if (prev_offset >= 0) {
                ShmKeyValue* prev_kv = reinterpret_cast<ShmKeyValue*>(
                    static_cast<uint8_t*>(base) + prev_offset
                );
                prev_kv->next_offset.store(new_offset, std::memory_order_release);
            } else {
                bucket->head_offset.store(new_offset, std::memory_order_release);
            }
            
            if (next_offset >= 0) {
                ShmKeyValue* next_kv = reinterpret_cast<ShmKeyValue*>(
                    static_cast<uint8_t*>(base) + next_offset
                );
                next_kv->prev_offset.store(new_offset, std::memory_order_release);
            }
            
            existing->entry.mark_deleted();
            free_kv(existing);
        }
        
        header_->modified_at = current_timestamp_ns();
        stats_.write_count.fetch_add(1, std::memory_order_relaxed);
        return true;
    }
    
    // Add new entry
    ShmKeyValue* kv = allocate_kv(key_size, value_size);
    SerializationUtil::copy_to_kv(kv, key, key_size, value, value_size, ttl_seconds);
    
    int64_t kv_offset = static_cast<uint8_t*>(static_cast<void*>(kv)) - 
                        static_cast<uint8_t*>(base);
    
    int64_t old_head = bucket->head_offset.load(std::memory_order_acquire);
    kv->next_offset.store(old_head, std::memory_order_release);
    kv->prev_offset.store(ShmKeyValue::NULL_OFFSET, std::memory_order_release);
    
    if (old_head >= 0) {
        ShmKeyValue* old_head_kv = reinterpret_cast<ShmKeyValue*>(
            static_cast<uint8_t*>(base) + old_head
        );
        old_head_kv->prev_offset.store(kv_offset, std::memory_order_release);
    }
    
    bucket->head_offset.store(kv_offset, std::memory_order_release);
    bucket->size.fetch_add(1, std::memory_order_acq_rel);
    
    header_->size.fetch_add(1, std::memory_order_acq_rel);
    header_->modified_at = current_timestamp_ns();
    stats_.size.fetch_add(1, std::memory_order_relaxed);
    stats_.write_count.fetch_add(1, std::memory_order_relaxed);
    
    return true;
}

bool FastMap::putIfAbsent(const uint8_t* key, size_t key_size,
                          const uint8_t* value, size_t value_size,
                          int32_t ttl_seconds) {
    if (!key || key_size == 0) return false;
    
    uint32_t hash = compute_hash(key, key_size);
    ShmBucket* bucket = get_bucket(hash);
    
    IpcScopedLock lock(bucket->mutex);
    
    ShmKeyValue* existing = find_in_bucket(bucket, key, key_size, hash, nullptr);
    if (existing && existing->entry.is_alive()) {
        return false;  // Key already exists
    }
    
    // If expired, remove it first
    if (existing) {
        void* base = file_manager_->segment_manager();
        int64_t prev = existing->prev_offset.load(std::memory_order_acquire);
        int64_t next = existing->next_offset.load(std::memory_order_acquire);
        
        if (prev >= 0) {
            ShmKeyValue* prev_kv = reinterpret_cast<ShmKeyValue*>(
                static_cast<uint8_t*>(base) + prev
            );
            prev_kv->next_offset.store(next, std::memory_order_release);
        } else {
            bucket->head_offset.store(next, std::memory_order_release);
        }
        
        if (next >= 0) {
            ShmKeyValue* next_kv = reinterpret_cast<ShmKeyValue*>(
                static_cast<uint8_t*>(base) + next
            );
            next_kv->prev_offset.store(prev, std::memory_order_release);
        }
        
        existing->entry.mark_deleted();
        free_kv(existing);
        bucket->size.fetch_sub(1, std::memory_order_acq_rel);
        header_->size.fetch_sub(1, std::memory_order_acq_rel);
        stats_.size.fetch_sub(1, std::memory_order_relaxed);
    }
    
    // Add new entry
    void* base = file_manager_->segment_manager();
    ShmKeyValue* kv = allocate_kv(key_size, value_size);
    SerializationUtil::copy_to_kv(kv, key, key_size, value, value_size, ttl_seconds);
    
    int64_t kv_offset = static_cast<uint8_t*>(static_cast<void*>(kv)) - 
                        static_cast<uint8_t*>(base);
    
    int64_t old_head = bucket->head_offset.load(std::memory_order_acquire);
    kv->next_offset.store(old_head, std::memory_order_release);
    kv->prev_offset.store(ShmKeyValue::NULL_OFFSET, std::memory_order_release);
    
    if (old_head >= 0) {
        ShmKeyValue* old_head_kv = reinterpret_cast<ShmKeyValue*>(
            static_cast<uint8_t*>(base) + old_head
        );
        old_head_kv->prev_offset.store(kv_offset, std::memory_order_release);
    }
    
    bucket->head_offset.store(kv_offset, std::memory_order_release);
    bucket->size.fetch_add(1, std::memory_order_acq_rel);
    
    header_->size.fetch_add(1, std::memory_order_acq_rel);
    header_->modified_at = current_timestamp_ns();
    stats_.size.fetch_add(1, std::memory_order_relaxed);
    stats_.write_count.fetch_add(1, std::memory_order_relaxed);
    
    return true;
}

bool FastMap::get(const uint8_t* key, size_t key_size,
                  std::vector<uint8_t>& out_value) const {
    if (!key || key_size == 0) return false;
    
    uint32_t hash = compute_hash(key, key_size);
    const ShmBucket* bucket = get_bucket(hash);
    
    void* base = file_manager_->segment_manager();
    int64_t current = bucket->head_offset.load(std::memory_order_acquire);
    
    while (current >= 0) {
        const ShmKeyValue* kv = reinterpret_cast<const ShmKeyValue*>(
            static_cast<const uint8_t*>(base) + current
        );
        
        if (kv->entry.is_alive() &&
            kv->entry.hash_code == hash &&
            kv->key_size == key_size &&
            std::memcmp(kv->data, key, key_size) == 0) {
            
            out_value.resize(kv->value_size);
            std::memcpy(out_value.data(), kv->data + kv->key_size, kv->value_size);
            
            const_cast<CollectionStats&>(stats_).hit_count.fetch_add(1, std::memory_order_relaxed);
            const_cast<CollectionStats&>(stats_).read_count.fetch_add(1, std::memory_order_relaxed);
            return true;
        }
        
        current = kv->next_offset.load(std::memory_order_acquire);
    }
    
    const_cast<CollectionStats&>(stats_).miss_count.fetch_add(1, std::memory_order_relaxed);
    const_cast<CollectionStats&>(stats_).read_count.fetch_add(1, std::memory_order_relaxed);
    return false;
}

std::vector<uint8_t> FastMap::getOrDefault(const uint8_t* key, size_t key_size,
                                            const std::vector<uint8_t>& default_value) const {
    std::vector<uint8_t> result;
    if (get(key, key_size, result)) {
        return result;
    }
    return default_value;
}

int64_t FastMap::getTTL(const uint8_t* key, size_t key_size) const {
    if (!key || key_size == 0) return 0;
    
    uint32_t hash = compute_hash(key, key_size);
    const ShmBucket* bucket = get_bucket(hash);
    
    void* base = file_manager_->segment_manager();
    int64_t current = bucket->head_offset.load(std::memory_order_acquire);
    
    while (current >= 0) {
        const ShmKeyValue* kv = reinterpret_cast<const ShmKeyValue*>(
            static_cast<const uint8_t*>(base) + current
        );
        
        if (kv->entry.is_alive() &&
            kv->entry.hash_code == hash &&
            kv->key_size == key_size &&
            std::memcmp(kv->data, key, key_size) == 0) {
            return kv->entry.remaining_ttl_seconds();
        }
        
        current = kv->next_offset.load(std::memory_order_acquire);
    }
    
    return 0;
}

bool FastMap::remove(const uint8_t* key, size_t key_size,
                     std::vector<uint8_t>* out_value) {
    if (!key || key_size == 0) return false;
    
    uint32_t hash = compute_hash(key, key_size);
    ShmBucket* bucket = get_bucket(hash);
    
    IpcScopedLock lock(bucket->mutex);
    
    void* base = file_manager_->segment_manager();
    ShmKeyValue* prev = nullptr;
    ShmKeyValue* kv = find_in_bucket(bucket, key, key_size, hash, &prev);
    
    if (!kv) return false;
    
    if (out_value && kv->entry.is_alive()) {
        out_value->resize(kv->value_size);
        std::memcpy(out_value->data(), kv->data + kv->key_size, kv->value_size);
    }
    
    int64_t next = kv->next_offset.load(std::memory_order_acquire);
    
    if (prev) {
        prev->next_offset.store(next, std::memory_order_release);
    } else {
        bucket->head_offset.store(next, std::memory_order_release);
    }
    
    if (next >= 0) {
        ShmKeyValue* next_kv = reinterpret_cast<ShmKeyValue*>(
            static_cast<uint8_t*>(base) + next
        );
        next_kv->prev_offset.store(
            prev ? (static_cast<uint8_t*>(static_cast<void*>(prev)) - 
                   static_cast<uint8_t*>(base)) : ShmKeyValue::NULL_OFFSET,
            std::memory_order_release
        );
    }
    
    kv->entry.mark_deleted();
    free_kv(kv);
    
    bucket->size.fetch_sub(1, std::memory_order_acq_rel);
    header_->size.fetch_sub(1, std::memory_order_acq_rel);
    header_->modified_at = current_timestamp_ns();
    stats_.size.fetch_sub(1, std::memory_order_relaxed);
    
    return true;
}

bool FastMap::remove(const uint8_t* key, size_t key_size,
                     const uint8_t* expected_value, size_t value_size) {
    if (!key || key_size == 0) return false;
    
    uint32_t hash = compute_hash(key, key_size);
    ShmBucket* bucket = get_bucket(hash);
    
    IpcScopedLock lock(bucket->mutex);
    
    void* base = file_manager_->segment_manager();
    ShmKeyValue* prev = nullptr;
    ShmKeyValue* kv = find_in_bucket(bucket, key, key_size, hash, &prev);
    
    if (!kv || !kv->entry.is_alive()) return false;
    
    // Check if value matches
    if (kv->value_size != value_size ||
        std::memcmp(kv->data + kv->key_size, expected_value, value_size) != 0) {
        return false;
    }
    
    int64_t next = kv->next_offset.load(std::memory_order_acquire);
    
    if (prev) {
        prev->next_offset.store(next, std::memory_order_release);
    } else {
        bucket->head_offset.store(next, std::memory_order_release);
    }
    
    if (next >= 0) {
        ShmKeyValue* next_kv = reinterpret_cast<ShmKeyValue*>(
            static_cast<uint8_t*>(base) + next
        );
        next_kv->prev_offset.store(
            prev ? (static_cast<uint8_t*>(static_cast<void*>(prev)) - 
                   static_cast<uint8_t*>(base)) : ShmKeyValue::NULL_OFFSET,
            std::memory_order_release
        );
    }
    
    kv->entry.mark_deleted();
    free_kv(kv);
    
    bucket->size.fetch_sub(1, std::memory_order_acq_rel);
    header_->size.fetch_sub(1, std::memory_order_acq_rel);
    header_->modified_at = current_timestamp_ns();
    stats_.size.fetch_sub(1, std::memory_order_relaxed);
    
    return true;
}

size_t FastMap::removeExpired() {
    size_t removed = 0;
    void* base = file_manager_->segment_manager();
    
    for (uint32_t i = 0; i < header_->bucket_count; i++) {
        ShmBucket* bucket = &buckets_[i];
        IpcScopedLock lock(bucket->mutex);
        
        int64_t current = bucket->head_offset.load(std::memory_order_acquire);
        
        while (current >= 0) {
            ShmKeyValue* kv = reinterpret_cast<ShmKeyValue*>(
                static_cast<uint8_t*>(base) + current
            );
            int64_t next = kv->next_offset.load(std::memory_order_acquire);
            
            if (kv->entry.is_expired()) {
                int64_t prev = kv->prev_offset.load(std::memory_order_acquire);
                
                if (prev >= 0) {
                    ShmKeyValue* prev_kv = reinterpret_cast<ShmKeyValue*>(
                        static_cast<uint8_t*>(base) + prev
                    );
                    prev_kv->next_offset.store(next, std::memory_order_release);
                } else {
                    bucket->head_offset.store(next, std::memory_order_release);
                }
                
                if (next >= 0) {
                    ShmKeyValue* next_kv = reinterpret_cast<ShmKeyValue*>(
                        static_cast<uint8_t*>(base) + next
                    );
                    next_kv->prev_offset.store(prev, std::memory_order_release);
                }
                
                kv->entry.mark_deleted();
                free_kv(kv);
                
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

bool FastMap::replace(const uint8_t* key, size_t key_size,
                      const uint8_t* value, size_t value_size,
                      int32_t ttl_seconds) {
    if (!key || key_size == 0) return false;
    
    uint32_t hash = compute_hash(key, key_size);
    ShmBucket* bucket = get_bucket(hash);
    
    IpcScopedLock lock(bucket->mutex);
    
    ShmKeyValue* kv = find_in_bucket(bucket, key, key_size, hash, nullptr);
    if (!kv || !kv->entry.is_alive()) {
        return false;
    }
    
    void* base = file_manager_->segment_manager();
    
    if (kv->value_size == value_size) {
        // Same size - update in place
        std::memcpy(kv->data + key_size, value, value_size);
        kv->entry.set_ttl(ttl_seconds);
    } else {
        // Different size - reallocate
        int64_t prev_offset = kv->prev_offset.load(std::memory_order_acquire);
        int64_t next_offset = kv->next_offset.load(std::memory_order_acquire);
        
        ShmKeyValue* new_kv = allocate_kv(key_size, value_size);
        SerializationUtil::copy_to_kv(new_kv, key, key_size, value, value_size, ttl_seconds);
        
        int64_t new_offset = static_cast<uint8_t*>(static_cast<void*>(new_kv)) - 
                             static_cast<uint8_t*>(base);
        
        new_kv->prev_offset.store(prev_offset, std::memory_order_release);
        new_kv->next_offset.store(next_offset, std::memory_order_release);
        
        if (prev_offset >= 0) {
            ShmKeyValue* prev_kv = reinterpret_cast<ShmKeyValue*>(
                static_cast<uint8_t*>(base) + prev_offset
            );
            prev_kv->next_offset.store(new_offset, std::memory_order_release);
        } else {
            bucket->head_offset.store(new_offset, std::memory_order_release);
        }
        
        if (next_offset >= 0) {
            ShmKeyValue* next_kv = reinterpret_cast<ShmKeyValue*>(
                static_cast<uint8_t*>(base) + next_offset
            );
            next_kv->prev_offset.store(new_offset, std::memory_order_release);
        }
        
        kv->entry.mark_deleted();
        free_kv(kv);
    }
    
    header_->modified_at = current_timestamp_ns();
    stats_.write_count.fetch_add(1, std::memory_order_relaxed);
    
    return true;
}

bool FastMap::replace(const uint8_t* key, size_t key_size,
                      const uint8_t* old_value, size_t old_value_size,
                      const uint8_t* new_value, size_t new_value_size,
                      int32_t ttl_seconds) {
    if (!key || key_size == 0) return false;
    
    uint32_t hash = compute_hash(key, key_size);
    ShmBucket* bucket = get_bucket(hash);
    
    IpcScopedLock lock(bucket->mutex);
    
    ShmKeyValue* kv = find_in_bucket(bucket, key, key_size, hash, nullptr);
    if (!kv || !kv->entry.is_alive()) {
        return false;
    }
    
    // Check if current value matches expected
    if (kv->value_size != old_value_size ||
        std::memcmp(kv->data + kv->key_size, old_value, old_value_size) != 0) {
        return false;
    }
    
    void* base = file_manager_->segment_manager();
    
    if (kv->value_size == new_value_size) {
        std::memcpy(kv->data + key_size, new_value, new_value_size);
        kv->entry.set_ttl(ttl_seconds);
    } else {
        int64_t prev_offset = kv->prev_offset.load(std::memory_order_acquire);
        int64_t next_offset = kv->next_offset.load(std::memory_order_acquire);
        
        ShmKeyValue* new_kv = allocate_kv(key_size, new_value_size);
        SerializationUtil::copy_to_kv(new_kv, key, key_size, new_value, new_value_size, ttl_seconds);
        
        int64_t new_offset = static_cast<uint8_t*>(static_cast<void*>(new_kv)) - 
                             static_cast<uint8_t*>(base);
        
        new_kv->prev_offset.store(prev_offset, std::memory_order_release);
        new_kv->next_offset.store(next_offset, std::memory_order_release);
        
        if (prev_offset >= 0) {
            ShmKeyValue* prev_kv = reinterpret_cast<ShmKeyValue*>(
                static_cast<uint8_t*>(base) + prev_offset
            );
            prev_kv->next_offset.store(new_offset, std::memory_order_release);
        } else {
            bucket->head_offset.store(new_offset, std::memory_order_release);
        }
        
        if (next_offset >= 0) {
            ShmKeyValue* next_kv = reinterpret_cast<ShmKeyValue*>(
                static_cast<uint8_t*>(base) + next_offset
            );
            next_kv->prev_offset.store(new_offset, std::memory_order_release);
        }
        
        kv->entry.mark_deleted();
        free_kv(kv);
    }
    
    header_->modified_at = current_timestamp_ns();
    stats_.write_count.fetch_add(1, std::memory_order_relaxed);
    
    return true;
}

bool FastMap::setTTL(const uint8_t* key, size_t key_size, int32_t ttl_seconds) {
    if (!key || key_size == 0) return false;
    
    uint32_t hash = compute_hash(key, key_size);
    ShmBucket* bucket = get_bucket(hash);
    
    IpcScopedLock lock(bucket->mutex);
    
    ShmKeyValue* kv = find_in_bucket(bucket, key, key_size, hash, nullptr);
    if (!kv || !kv->entry.is_alive()) {
        return false;
    }
    
    kv->entry.set_ttl(ttl_seconds);
    header_->modified_at = current_timestamp_ns();
    
    return true;
}

bool FastMap::containsKey(const uint8_t* key, size_t key_size) const {
    if (!key || key_size == 0) return false;
    
    uint32_t hash = compute_hash(key, key_size);
    const ShmBucket* bucket = get_bucket(hash);
    
    void* base = file_manager_->segment_manager();
    int64_t current = bucket->head_offset.load(std::memory_order_acquire);
    
    while (current >= 0) {
        const ShmKeyValue* kv = reinterpret_cast<const ShmKeyValue*>(
            static_cast<const uint8_t*>(base) + current
        );
        
        if (kv->entry.is_alive() &&
            kv->entry.hash_code == hash &&
            kv->key_size == key_size &&
            std::memcmp(kv->data, key, key_size) == 0) {
            return true;
        }
        
        current = kv->next_offset.load(std::memory_order_acquire);
    }
    
    return false;
}

bool FastMap::containsValue(const uint8_t* value, size_t value_size) const {
    void* base = file_manager_->segment_manager();
    
    for (uint32_t i = 0; i < header_->bucket_count; i++) {
        const ShmBucket* bucket = &buckets_[i];
        int64_t current = bucket->head_offset.load(std::memory_order_acquire);
        
        while (current >= 0) {
            const ShmKeyValue* kv = reinterpret_cast<const ShmKeyValue*>(
                static_cast<const uint8_t*>(base) + current
            );
            
            if (kv->entry.is_alive() &&
                kv->value_size == value_size &&
                std::memcmp(kv->data + kv->key_size, value, value_size) == 0) {
                return true;
            }
            
            current = kv->next_offset.load(std::memory_order_acquire);
        }
    }
    
    return false;
}

void FastMap::forEach(std::function<bool(const uint8_t* key, size_t key_size,
                                          const uint8_t* value, size_t value_size)> callback) const {
    void* base = file_manager_->segment_manager();
    
    for (uint32_t i = 0; i < header_->bucket_count; i++) {
        const ShmBucket* bucket = &buckets_[i];
        int64_t current = bucket->head_offset.load(std::memory_order_acquire);
        
        while (current >= 0) {
            const ShmKeyValue* kv = reinterpret_cast<const ShmKeyValue*>(
                static_cast<const uint8_t*>(base) + current
            );
            
            if (kv->entry.is_alive()) {
                if (!callback(kv->data, kv->key_size, 
                             kv->data + kv->key_size, kv->value_size)) {
                    return;
                }
            }
            
            current = kv->next_offset.load(std::memory_order_acquire);
        }
    }
}

void FastMap::forEachWithTTL(std::function<bool(const uint8_t* key, size_t key_size,
                                                 const uint8_t* value, size_t value_size,
                                                 int64_t ttl_remaining)> callback) const {
    void* base = file_manager_->segment_manager();
    
    for (uint32_t i = 0; i < header_->bucket_count; i++) {
        const ShmBucket* bucket = &buckets_[i];
        int64_t current = bucket->head_offset.load(std::memory_order_acquire);
        
        while (current >= 0) {
            const ShmKeyValue* kv = reinterpret_cast<const ShmKeyValue*>(
                static_cast<const uint8_t*>(base) + current
            );
            
            if (kv->entry.is_alive()) {
                int64_t ttl = kv->entry.remaining_ttl_seconds();
                if (!callback(kv->data, kv->key_size,
                             kv->data + kv->key_size, kv->value_size, ttl)) {
                    return;
                }
            }
            
            current = kv->next_offset.load(std::memory_order_acquire);
        }
    }
}

void FastMap::forEachKey(std::function<bool(const uint8_t* key, size_t key_size)> callback) const {
    forEach([&callback](const uint8_t* key, size_t key_size,
                        const uint8_t*, size_t) {
        return callback(key, key_size);
    });
}

void FastMap::forEachValue(std::function<bool(const uint8_t* value, size_t value_size)> callback) const {
    forEach([&callback](const uint8_t*, size_t,
                        const uint8_t* value, size_t value_size) {
        return callback(value, value_size);
    });
}

std::vector<std::vector<uint8_t>> FastMap::keySet() const {
    std::vector<std::vector<uint8_t>> keys;
    keys.reserve(header_->size.load(std::memory_order_acquire));
    
    forEachKey([&keys](const uint8_t* key, size_t key_size) {
        keys.emplace_back(key, key + key_size);
        return true;
    });
    
    return keys;
}

std::vector<std::vector<uint8_t>> FastMap::values() const {
    std::vector<std::vector<uint8_t>> vals;
    vals.reserve(header_->size.load(std::memory_order_acquire));
    
    forEachValue([&vals](const uint8_t* value, size_t value_size) {
        vals.emplace_back(value, value + value_size);
        return true;
    });
    
    return vals;
}

void FastMap::clear() {
    void* base = file_manager_->segment_manager();
    
    for (uint32_t i = 0; i < header_->bucket_count; i++) {
        ShmBucket* bucket = &buckets_[i];
        IpcScopedLock lock(bucket->mutex);
        
        int64_t current = bucket->head_offset.load(std::memory_order_acquire);
        
        while (current >= 0) {
            ShmKeyValue* kv = reinterpret_cast<ShmKeyValue*>(
                static_cast<uint8_t*>(base) + current
            );
            int64_t next = kv->next_offset.load(std::memory_order_acquire);
            
            kv->entry.mark_deleted();
            free_kv(kv);
            
            current = next;
        }
        
        bucket->head_offset.store(ShmBucket::NULL_OFFSET, std::memory_order_release);
        bucket->size.store(0, std::memory_order_release);
    }
    
    header_->size.store(0, std::memory_order_release);
    header_->modified_at = current_timestamp_ns();
    stats_.size.store(0, std::memory_order_relaxed);
}

size_t FastMap::size() const {
    size_t alive = 0;
    void* base = file_manager_->segment_manager();
    
    for (uint32_t i = 0; i < header_->bucket_count; i++) {
        const ShmBucket* bucket = &buckets_[i];
        int64_t current = bucket->head_offset.load(std::memory_order_acquire);
        
        while (current >= 0) {
            const ShmKeyValue* kv = reinterpret_cast<const ShmKeyValue*>(
                static_cast<const uint8_t*>(base) + current
            );
            if (kv->entry.is_alive()) alive++;
            current = kv->next_offset.load(std::memory_order_acquire);
        }
    }
    
    return alive;
}

bool FastMap::isEmpty() const {
    return size() == 0;
}

void FastMap::flush() {
    file_manager_->flush();
}

} // namespace fastcollection
