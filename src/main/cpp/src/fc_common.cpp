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
 * @file fc_common.cpp
 * @brief Implementation of common utilities for FastCollection
 */

#include "fc_common.h"
#include "fc_serialization.h"
#include <cstring>
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

namespace fastcollection {

MMapFileManager::MMapFileManager(const std::string& filename, 
                                  size_t initial_size,
                                  bool create_new)
    : filename_(filename)
    , growth_size_(DEFAULT_GROWTH_SIZE) {
    
    try {
        if (create_new || !fs::exists(filename)) {
            // Remove existing file if creating new
            if (fs::exists(filename)) {
                bip::file_mapping::remove(filename.c_str());
            }
            
            file_ = std::make_unique<bip::managed_mapped_file>(
                bip::create_only,
                filename.c_str(),
                initial_size
            );
        } else {
            // Open existing file
            file_ = std::make_unique<bip::managed_mapped_file>(
                bip::open_only,
                filename.c_str()
            );
        }
    } catch (const bip::interprocess_exception& e) {
        throw FastCollectionException(
            FastCollectionException::ErrorCode::FILE_CREATION_FAILED,
            std::string("Failed to create/open memory-mapped file: ") + e.what()
        );
    }
}

MMapFileManager::~MMapFileManager() {
    if (file_) {
        try {
            file_->flush();
        } catch (...) {
            // Ignore errors during destruction
        }
    }
}

MMapFileManager::MMapFileManager(MMapFileManager&& other) noexcept
    : filename_(std::move(other.filename_))
    , file_(std::move(other.file_))
    , growth_size_(other.growth_size_) {
}

MMapFileManager& MMapFileManager::operator=(MMapFileManager&& other) noexcept {
    if (this != &other) {
        filename_ = std::move(other.filename_);
        file_ = std::move(other.file_);
        growth_size_ = other.growth_size_;
    }
    return *this;
}

SegmentManager* MMapFileManager::segment_manager() {
    return file_->get_segment_manager();
}

void* MMapFileManager::allocate(size_t bytes) {
    try {
        return file_->allocate(bytes);
    } catch (const bip::bad_alloc&) {
        // Try to grow and retry
        if (grow(bytes + growth_size_)) {
            return file_->allocate(bytes);
        }
        throw FastCollectionException(
            FastCollectionException::ErrorCode::MEMORY_ALLOCATION_FAILED,
            "Failed to allocate memory in mapped file"
        );
    }
}

void MMapFileManager::deallocate(void* ptr) {
    if (ptr) {
        file_->deallocate(ptr);
    }
}

bool MMapFileManager::grow(size_t additional_bytes) {
    try {
        // Close current mapping
        file_.reset();
        
        // Grow the file
        bip::managed_mapped_file::grow(filename_.c_str(), additional_bytes);
        
        // Reopen
        file_ = std::make_unique<bip::managed_mapped_file>(
            bip::open_only,
            filename_.c_str()
        );
        
        return true;
    } catch (const bip::interprocess_exception&) {
        // Reopen without growing
        file_ = std::make_unique<bip::managed_mapped_file>(
            bip::open_only,
            filename_.c_str()
        );
        return false;
    }
}

size_t MMapFileManager::free_space() const {
    return file_->get_free_memory();
}

size_t MMapFileManager::size() const {
    return file_->get_size();
}

void MMapFileManager::flush() {
    file_->flush();
}

// Library functions
static bool g_initialized = false;

void initialize() {
    if (!g_initialized) {
        g_initialized = true;
    }
}

void shutdown() {
    g_initialized = false;
}

const char* version() {
    return FASTCOLLECTION_VERSION_STRING;
}

bool deleteCollectionFile(const std::string& filename) {
    try {
        return bip::file_mapping::remove(filename.c_str());
    } catch (...) {
        return false;
    }
}

bool isValidCollectionFile(const std::string& filename) {
    try {
        if (!fs::exists(filename)) {
            return false;
        }
        
        bip::managed_mapped_file file(bip::open_read_only, filename.c_str());
        auto result = file.find<CollectionHeader>("header");
        if (result.first && result.first->is_valid()) {
            return true;
        }
        return false;
    } catch (...) {
        return false;
    }
}

bool getFileStats(const std::string& filename, FileStats& stats) {
    try {
        if (!fs::exists(filename)) {
            return false;
        }
        
        bip::managed_mapped_file file(bip::open_read_only, filename.c_str());
        auto result = file.find<CollectionHeader>("header");
        
        if (!result.first || !result.first->is_valid()) {
            return false;
        }
        
        stats.total_size = file.get_size();
        stats.free_size = file.get_free_memory();
        stats.used_size = stats.total_size - stats.free_size;
        stats.element_count = static_cast<uint32_t>(result.first->size.load());
        stats.created_at = result.first->created_at;
        stats.modified_at = result.first->modified_at;
        
        return true;
    } catch (...) {
        return false;
    }
}

} // namespace fastcollection
