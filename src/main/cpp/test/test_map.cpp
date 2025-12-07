/**
 * Copyright © 2025-2030, All Rights Reserved
 * Ashutosh Sinha | Email: ajsinha@gmail.com
 * Patent Pending
 * 
 * @file test_map.cpp
 * @brief Tests for FastMap with TTL support
 */

#include "fastcollection.h"
#include <iostream>
#include <cassert>
#include <thread>
#include <chrono>

using namespace fastcollection;

void test_basic_operations() {
    std::cout << "Testing basic map operations..." << std::endl;
    
    FastMap map("/tmp/test_map.fc", 16 * 1024 * 1024, true);
    
    assert(map.isEmpty());
    assert(map.size() == 0);
    
    std::string key = "key1";
    std::string value = "value1";
    
    assert(map.put(reinterpret_cast<const uint8_t*>(key.data()), key.size(),
                   reinterpret_cast<const uint8_t*>(value.data()), value.size()));
    
    assert(!map.isEmpty());
    assert(map.size() == 1);
    
    std::vector<uint8_t> result;
    assert(map.get(reinterpret_cast<const uint8_t*>(key.data()), key.size(), result));
    assert(std::string(result.begin(), result.end()) == "value1");
    
    assert(map.containsKey(reinterpret_cast<const uint8_t*>(key.data()), key.size()));
    
    assert(map.remove(reinterpret_cast<const uint8_t*>(key.data()), key.size()));
    assert(map.isEmpty());
    
    std::cout << "  PASSED" << std::endl;
}

void test_ttl() {
    std::cout << "Testing map TTL..." << std::endl;
    
    FastMap map("/tmp/test_map_ttl.fc", 16 * 1024 * 1024, true);
    
    std::string key = "temp_key";
    std::string value = "temp_value";
    
    // Add with 1 second TTL
    map.put(reinterpret_cast<const uint8_t*>(key.data()), key.size(),
            reinterpret_cast<const uint8_t*>(value.data()), value.size(), 1);
    
    assert(map.size() == 1);
    
    // Wait for expiration
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Should be expired
    assert(map.size() == 0);
    
    std::cout << "  PASSED" << std::endl;
}

void test_put_if_absent() {
    std::cout << "Testing putIfAbsent..." << std::endl;
    
    FastMap map("/tmp/test_map_pia.fc", 16 * 1024 * 1024, true);
    
    std::string key = "key";
    std::string val1 = "first";
    std::string val2 = "second";
    
    assert(map.putIfAbsent(reinterpret_cast<const uint8_t*>(key.data()), key.size(),
                           reinterpret_cast<const uint8_t*>(val1.data()), val1.size()));
    
    // Should fail - key exists
    assert(!map.putIfAbsent(reinterpret_cast<const uint8_t*>(key.data()), key.size(),
                            reinterpret_cast<const uint8_t*>(val2.data()), val2.size()));
    
    std::vector<uint8_t> result;
    map.get(reinterpret_cast<const uint8_t*>(key.data()), key.size(), result);
    assert(std::string(result.begin(), result.end()) == "first");
    
    std::cout << "  PASSED" << std::endl;
}

int main() {
    std::cout << "\n=== FastCollection Map Tests ===" << std::endl;
    
    try {
        test_basic_operations();
        test_ttl();
        test_put_if_absent();
        
        std::cout << "\n=== All tests PASSED ===" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test FAILED: " << e.what() << std::endl;
        return 1;
    }
}
