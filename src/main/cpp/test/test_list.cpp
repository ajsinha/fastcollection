/**
 * Copyright © 2025-2030, All Rights Reserved
 * Ashutosh Sinha | Email: ajsinha@gmail.com
 * Patent Pending
 * 
 * @file test_list.cpp
 * @brief Tests for FastList with TTL support
 */

#include "fastcollection.h"
#include <iostream>
#include <cassert>
#include <thread>
#include <chrono>
#include <cstring>

using namespace fastcollection;

void test_basic_operations() {
    std::cout << "Testing basic operations..." << std::endl;
    
    // Create list
    FastList list("/tmp/test_list.fc", 16 * 1024 * 1024, true);
    
    assert(list.isEmpty());
    assert(list.size() == 0);
    
    // Add without TTL (never expires)
    std::string data1 = "hello";
    assert(list.add(reinterpret_cast<const uint8_t*>(data1.data()), data1.size()));
    
    assert(!list.isEmpty());
    assert(list.size() == 1);
    
    // Get
    std::vector<uint8_t> result;
    assert(list.get(0, result));
    assert(std::string(result.begin(), result.end()) == "hello");
    
    // Add more
    std::string data2 = "world";
    list.add(reinterpret_cast<const uint8_t*>(data2.data()), data2.size());
    assert(list.size() == 2);
    
    // Remove
    list.remove(0);
    assert(list.size() == 1);
    
    // Clear
    list.clear();
    assert(list.isEmpty());
    
    std::cout << "  PASSED" << std::endl;
}

void test_ttl_infinite() {
    std::cout << "Testing TTL infinite (never expires)..." << std::endl;
    
    FastList list("/tmp/test_list_ttl.fc", 16 * 1024 * 1024, true);
    
    std::string data = "permanent";
    // TTL = -1 means never expires
    list.add(reinterpret_cast<const uint8_t*>(data.data()), data.size(), TTL_INFINITE);
    
    // Check TTL
    int64_t ttl = list.getTTL(0);
    assert(ttl == -1);  // -1 means infinite
    
    // Element should be accessible
    std::vector<uint8_t> result;
    assert(list.get(0, result));
    assert(std::string(result.begin(), result.end()) == "permanent");
    
    std::cout << "  PASSED" << std::endl;
}

void test_ttl_expiration() {
    std::cout << "Testing TTL expiration..." << std::endl;
    
    FastList list("/tmp/test_list_exp.fc", 16 * 1024 * 1024, true);
    
    std::string data = "temporary";
    // TTL = 1 second
    list.add(reinterpret_cast<const uint8_t*>(data.data()), data.size(), 1);
    
    // Should be accessible immediately
    std::vector<uint8_t> result;
    assert(list.get(0, result));
    
    // Check TTL (should be ~1 second)
    int64_t ttl = list.getTTL(0);
    assert(ttl >= 0 && ttl <= 1);
    
    // Wait for expiration
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Should be expired now
    ttl = list.getTTL(0);
    assert(ttl == 0);  // 0 means expired
    
    // Size should report 0 (expired elements don't count)
    assert(list.size() == 0);
    
    // Remove expired
    size_t removed = list.removeExpired();
    assert(removed == 1);
    
    std::cout << "  PASSED" << std::endl;
}

void test_ttl_update() {
    std::cout << "Testing TTL update..." << std::endl;
    
    FastList list("/tmp/test_list_upd.fc", 16 * 1024 * 1024, true);
    
    std::string data = "test";
    // Add with 10 second TTL
    list.add(reinterpret_cast<const uint8_t*>(data.data()), data.size(), 10);
    
    // Update TTL to 60 seconds
    assert(list.setTTL(0, 60));
    
    // Check updated TTL
    int64_t ttl = list.getTTL(0);
    assert(ttl > 50 && ttl <= 60);
    
    // Update to infinite
    assert(list.setTTL(0, TTL_INFINITE));
    ttl = list.getTTL(0);
    assert(ttl == -1);  // Infinite
    
    std::cout << "  PASSED" << std::endl;
}

void test_persistence() {
    std::cout << "Testing persistence..." << std::endl;
    
    const char* file = "/tmp/test_list_persist.fc";
    
    // Create and populate
    {
        FastList list(file, 16 * 1024 * 1024, true);
        
        std::string data = "persistent data";
        list.add(reinterpret_cast<const uint8_t*>(data.data()), data.size());
        list.flush();
    }
    
    // Reopen and verify
    {
        FastList list(file);
        
        assert(list.size() == 1);
        
        std::vector<uint8_t> result;
        assert(list.get(0, result));
        assert(std::string(result.begin(), result.end()) == "persistent data");
    }
    
    std::cout << "  PASSED" << std::endl;
}

void test_mixed_ttl() {
    std::cout << "Testing mixed TTL elements..." << std::endl;
    
    FastList list("/tmp/test_list_mix.fc", 16 * 1024 * 1024, true);
    
    // Add elements with different TTLs
    std::string perm = "permanent";
    std::string temp1 = "expires_soon";
    std::string temp2 = "expires_later";
    
    list.add(reinterpret_cast<const uint8_t*>(perm.data()), perm.size(), TTL_INFINITE);
    list.add(reinterpret_cast<const uint8_t*>(temp1.data()), temp1.size(), 1);  // 1 sec
    list.add(reinterpret_cast<const uint8_t*>(temp2.data()), temp2.size(), 60); // 60 sec
    
    assert(list.size() == 3);
    
    // Wait for first temp to expire
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Size should be 2 now
    assert(list.size() == 2);
    
    std::cout << "  PASSED" << std::endl;
}

int main() {
    std::cout << "\n=== FastCollection List Tests ===" << std::endl;
    std::cout << "TTL=-1 means element never expires (default)\n" << std::endl;
    
    try {
        test_basic_operations();
        test_ttl_infinite();
        test_ttl_expiration();
        test_ttl_update();
        test_persistence();
        test_mixed_ttl();
        
        std::cout << "\n=== All tests PASSED ===" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test FAILED: " << e.what() << std::endl;
        return 1;
    }
}
