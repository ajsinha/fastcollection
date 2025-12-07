/**
 * FastCollection v1.0.0 - Basic Example (C++)
 * 
 * This example demonstrates basic operations with FastList.
 * 
 * Compile:
 *   g++ -std=c++20 -O3 -I../src/main/cpp/include \
 *       basic_example.cpp -o basic_example \
 *       -L../target/native/linux/x64 -lfastcollection_cpp-linux-x64 \
 *       -lboost_system -lpthread -lrt
 * 
 * Run:
 *   LD_LIBRARY_PATH=../target/native/linux/x64 ./basic_example
 * 
 * Copyright © 2025-2030, Ashutosh Sinha (ajsinha@gmail.com)
 * Patent Pending
 */

#include "fastcollection.h"
#include <iostream>
#include <string>
#include <vector>

int main() {
    std::cout << "FastCollection v1.0.0 - Basic Example (C++)" << std::endl;
    std::cout << std::string(50, '=') << std::endl;
    std::cout << std::endl;
    
    using namespace fastcollection;
    
    try {
        // Create a persistent list
        const char* path = "/tmp/basic_example_cpp.fc";
        FastList list(path, 16 * 1024 * 1024, true);
        
        // Add elements
        std::cout << "Adding elements..." << std::endl;
        std::vector<std::string> items = {"Hello", "World", "FastCollection", "is", "awesome!"};
        
        for (const auto& item : items) {
            list.add(reinterpret_cast<const uint8_t*>(item.data()), item.size());
        }
        
        // Display size
        std::cout << "List size: " << list.size() << std::endl;
        
        // Access elements
        std::cout << std::endl << "Elements:" << std::endl;
        std::vector<uint8_t> result;
        for (size_t i = 0; i < list.size(); i++) {
            if (list.get(i, result)) {
                std::string value(result.begin(), result.end());
                std::cout << "  [" << i << "]: " << value << std::endl;
            }
            result.clear();
        }
        
        // Check contains
        std::string searchWorld = "World";
        std::string searchJava = "Java";
        
        bool containsWorld = list.contains(
            reinterpret_cast<const uint8_t*>(searchWorld.data()), searchWorld.size()
        );
        bool containsJava = list.contains(
            reinterpret_cast<const uint8_t*>(searchJava.data()), searchJava.size()
        );
        
        std::cout << std::endl;
        std::cout << "Contains 'World': " << (containsWorld ? "true" : "false") << std::endl;
        std::cout << "Contains 'Java': " << (containsJava ? "true" : "false") << std::endl;
        
        // Find index
        std::string searchFC = "FastCollection";
        int idx = list.indexOf(
            reinterpret_cast<const uint8_t*>(searchFC.data()), searchFC.size()
        );
        std::cout << "Index of 'FastCollection': " << idx << std::endl;
        
        // Remove element
        std::cout << std::endl << "Removing element at index 1..." << std::endl;
        if (list.remove(1, result)) {
            std::string removed(result.begin(), result.end());
            std::cout << "Removed: " << removed << std::endl;
        }
        
        // Display updated list
        std::cout << std::endl << "Updated list:" << std::endl;
        for (size_t i = 0; i < list.size(); i++) {
            result.clear();
            if (list.get(i, result)) {
                std::string value(result.begin(), result.end());
                std::cout << "  [" << i << "]: " << value << std::endl;
            }
        }
        
        // Clear list
        std::cout << std::endl << "Clearing list..." << std::endl;
        list.clear();
        std::cout << "List empty: " << (list.isEmpty() ? "true" : "false") << std::endl;
        
        std::cout << std::endl << "Example completed successfully!" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
