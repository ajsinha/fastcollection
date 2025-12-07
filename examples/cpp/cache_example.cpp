/**
 * FastCollection v1.0.0 - Cache Example (C++)
 * 
 * Demonstrates using FastMap as a key-value cache with TTL.
 * 
 * Compile:
 *   g++ -std=c++20 -O3 -I../src/main/cpp/include \
 *       cache_example.cpp -o cache_example \
 *       -L../target/native/linux/x64 -lfastcollection_cpp-linux-x64 \
 *       -lboost_system -lpthread -lrt
 * 
 * Copyright © 2025-2030, Ashutosh Sinha (ajsinha@gmail.com)
 * Patent Pending
 */

#include "fastcollection.h"
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>

using namespace fastcollection;

/**
 * Simple cache wrapper around FastMap
 */
class Cache {
private:
    FastMap store;
    int defaultTTL;
    
public:
    Cache(const std::string& path, int defaultTTLSeconds = 300)
        : store(path, 64 * 1024 * 1024, true)
        , defaultTTL(defaultTTLSeconds) {}
    
    void put(const std::string& key, const std::string& value) {
        put(key, value, defaultTTL);
    }
    
    void put(const std::string& key, const std::string& value, int ttl) {
        store.put(
            reinterpret_cast<const uint8_t*>(key.data()), key.size(),
            reinterpret_cast<const uint8_t*>(value.data()), value.size(),
            ttl
        );
    }
    
    bool get(const std::string& key, std::string& value) {
        std::vector<uint8_t> result;
        if (store.get(reinterpret_cast<const uint8_t*>(key.data()), key.size(), result)) {
            value.assign(result.begin(), result.end());
            return true;
        }
        return false;
    }
    
    std::string getOrDefault(const std::string& key, const std::string& defaultValue) {
        std::string value;
        if (get(key, value)) {
            return value;
        }
        return defaultValue;
    }
    
    bool contains(const std::string& key) {
        return store.containsKey(
            reinterpret_cast<const uint8_t*>(key.data()), key.size()
        );
    }
    
    void remove(const std::string& key) {
        store.remove(
            reinterpret_cast<const uint8_t*>(key.data()), key.size()
        );
    }
    
    int64_t getTTL(const std::string& key) {
        return store.getTTL(
            reinterpret_cast<const uint8_t*>(key.data()), key.size()
        );
    }
    
    void setTTL(const std::string& key, int ttl) {
        store.setTTL(
            reinterpret_cast<const uint8_t*>(key.data()), key.size(),
            ttl
        );
    }
    
    size_t size() {
        return store.size();
    }
    
    int cleanup() {
        return store.removeExpired();
    }
};

int main() {
    std::cout << "FastCollection v1.0.0 - Cache Example (C++)" << std::endl;
    std::cout << std::string(50, '=') << std::endl;
    std::cout << std::endl;
    
    try {
        // Create cache with 10-second default TTL
        Cache cache("/tmp/cache_example_cpp.fc", 10);
        
        // Store user data
        std::cout << "Storing user data..." << std::endl;
        cache.put("user:1001", "John Doe");
        cache.put("user:1002", "Jane Smith");
        cache.put("user:1003", "Bob Johnson");
        
        // Store with custom TTL
        cache.put("session:abc123", "session_data", 30);  // 30 seconds
        cache.put("config:app", "config_value", -1);      // Never expires
        
        std::cout << "Cache size: " << cache.size() << std::endl;
        
        // Retrieve data
        std::cout << std::endl << "Retrieving data:" << std::endl;
        
        std::string value;
        if (cache.get("user:1001", value)) {
            std::cout << "  user:1001 = " << value << std::endl;
        }
        
        if (cache.get("user:1002", value)) {
            std::cout << "  user:1002 = " << value << std::endl;
        }
        
        std::cout << "  nonexistent = " << cache.getOrDefault("nonexistent", "DEFAULT") << std::endl;
        
        // Check TTLs
        std::cout << std::endl << "TTL values:" << std::endl;
        std::cout << "  user:1001 TTL = " << cache.getTTL("user:1001") << "s" << std::endl;
        std::cout << "  session:abc123 TTL = " << cache.getTTL("session:abc123") << "s" << std::endl;
        std::cout << "  config:app TTL = " << cache.getTTL("config:app") << " (infinite)" << std::endl;
        
        // Wait for some items to expire
        std::cout << std::endl << "Waiting 12 seconds for user data to expire..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(12));
        
        // Check what's left
        std::cout << std::endl << "After expiry:" << std::endl;
        std::cout << "  Cache size: " << cache.size() << std::endl;
        
        if (cache.get("user:1001", value)) {
            std::cout << "  user:1001 = " << value << std::endl;
        } else {
            std::cout << "  user:1001 = (expired)" << std::endl;
        }
        
        if (cache.get("session:abc123", value)) {
            std::cout << "  session:abc123 = " << value << std::endl;
        }
        
        if (cache.get("config:app", value)) {
            std::cout << "  config:app = " << value << std::endl;
        }
        
        // Cleanup expired entries
        int removed = cache.cleanup();
        std::cout << std::endl << "Cleaned up " << removed << " expired entries" << std::endl;
        
        std::cout << std::endl << "Example completed successfully!" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
