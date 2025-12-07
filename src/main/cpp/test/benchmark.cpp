/**
 * Copyright © 2025-2030, All Rights Reserved
 * Ashutosh Sinha | Email: ajsinha@gmail.com
 * Patent Pending
 * 
 * @file benchmark.cpp
 * @brief Performance benchmarks for FastCollection
 */

#include "fastcollection.h"
#include <iostream>
#include <chrono>
#include <vector>
#include <random>
#include <iomanip>

using namespace fastcollection;
using namespace std::chrono;

class Timer {
    high_resolution_clock::time_point start_;
public:
    Timer() : start_(high_resolution_clock::now()) {}
    
    double elapsed_ms() const {
        auto end = high_resolution_clock::now();
        return duration_cast<nanoseconds>(end - start_).count() / 1e6;
    }
    
    double ops_per_sec(size_t ops) const {
        double ms = elapsed_ms();
        return (ops / ms) * 1000.0;
    }
};

void benchmark_list(size_t ops) {
    std::cout << "\n=== FastList Benchmark ===" << std::endl;
    
    FastList list("/tmp/bench_list.fc", 256 * 1024 * 1024, true);
    std::vector<uint8_t> data(100, 'X');  // 100-byte payload
    
    // Add operations
    {
        Timer t;
        for (size_t i = 0; i < ops; ++i) {
            list.add(data.data(), data.size());
        }
        std::cout << "  Add: " << std::fixed << std::setprecision(0) 
                  << t.ops_per_sec(ops) << " ops/sec" << std::endl;
    }
    
    // Get operations
    {
        std::vector<uint8_t> result;
        Timer t;
        for (size_t i = 0; i < ops; ++i) {
            list.get(i % list.size(), result);
        }
        std::cout << "  Get: " << std::fixed << std::setprecision(0) 
                  << t.ops_per_sec(ops) << " ops/sec" << std::endl;
    }
}

void benchmark_map(size_t ops) {
    std::cout << "\n=== FastMap Benchmark ===" << std::endl;
    
    FastMap map("/tmp/bench_map.fc", 256 * 1024 * 1024, true);
    std::vector<uint8_t> value(100, 'V');
    
    // Put operations
    {
        Timer t;
        for (size_t i = 0; i < ops; ++i) {
            std::string key = "key_" + std::to_string(i);
            map.put(reinterpret_cast<const uint8_t*>(key.data()), key.size(),
                    value.data(), value.size());
        }
        std::cout << "  Put: " << std::fixed << std::setprecision(0) 
                  << t.ops_per_sec(ops) << " ops/sec" << std::endl;
    }
    
    // Get operations
    {
        std::vector<uint8_t> result;
        Timer t;
        for (size_t i = 0; i < ops; ++i) {
            std::string key = "key_" + std::to_string(i % ops);
            map.get(reinterpret_cast<const uint8_t*>(key.data()), key.size(), result);
        }
        std::cout << "  Get: " << std::fixed << std::setprecision(0) 
                  << t.ops_per_sec(ops) << " ops/sec" << std::endl;
    }
}

void benchmark_queue(size_t ops) {
    std::cout << "\n=== FastQueue Benchmark ===" << std::endl;
    
    FastQueue queue("/tmp/bench_queue.fc", 256 * 1024 * 1024, true);
    std::vector<uint8_t> data(100, 'Q');
    
    // Offer operations
    {
        Timer t;
        for (size_t i = 0; i < ops; ++i) {
            queue.offer(data.data(), data.size());
        }
        std::cout << "  Offer: " << std::fixed << std::setprecision(0) 
                  << t.ops_per_sec(ops) << " ops/sec" << std::endl;
    }
    
    // Poll operations
    {
        std::vector<uint8_t> result;
        Timer t;
        for (size_t i = 0; i < ops; ++i) {
            queue.poll(result);
        }
        std::cout << "  Poll: " << std::fixed << std::setprecision(0) 
                  << t.ops_per_sec(ops) << " ops/sec" << std::endl;
    }
}

void benchmark_stack(size_t ops) {
    std::cout << "\n=== FastStack Benchmark ===" << std::endl;
    
    FastStack stack("/tmp/bench_stack.fc", 256 * 1024 * 1024, true);
    std::vector<uint8_t> data(100, 'S');
    
    // Push operations
    {
        Timer t;
        for (size_t i = 0; i < ops; ++i) {
            stack.push(data.data(), data.size());
        }
        std::cout << "  Push: " << std::fixed << std::setprecision(0) 
                  << t.ops_per_sec(ops) << " ops/sec" << std::endl;
    }
    
    // Pop operations
    {
        std::vector<uint8_t> result;
        Timer t;
        for (size_t i = 0; i < ops; ++i) {
            stack.pop(result);
        }
        std::cout << "  Pop: " << std::fixed << std::setprecision(0) 
                  << t.ops_per_sec(ops) << " ops/sec" << std::endl;
    }
}

void benchmark_set(size_t ops) {
    std::cout << "\n=== FastSet Benchmark ===" << std::endl;
    
    FastSet set("/tmp/bench_set.fc", 256 * 1024 * 1024, true);
    
    // Add operations
    {
        Timer t;
        for (size_t i = 0; i < ops; ++i) {
            std::string data = "element_" + std::to_string(i);
            set.add(reinterpret_cast<const uint8_t*>(data.data()), data.size());
        }
        std::cout << "  Add: " << std::fixed << std::setprecision(0) 
                  << t.ops_per_sec(ops) << " ops/sec" << std::endl;
    }
    
    // Contains operations
    {
        Timer t;
        for (size_t i = 0; i < ops; ++i) {
            std::string data = "element_" + std::to_string(i % ops);
            set.contains(reinterpret_cast<const uint8_t*>(data.data()), data.size());
        }
        std::cout << "  Contains: " << std::fixed << std::setprecision(0) 
                  << t.ops_per_sec(ops) << " ops/sec" << std::endl;
    }
}

int main(int argc, char* argv[]) {
    size_t ops = 100000;
    if (argc > 1) {
        ops = std::stoul(argv[1]);
    }
    
    std::cout << "FastCollection Benchmark" << std::endl;
    std::cout << "Operations per test: " << ops << std::endl;
    std::cout << "Payload size: 100 bytes" << std::endl;
    
    benchmark_list(ops);
    benchmark_map(ops);
    benchmark_queue(ops);
    benchmark_stack(ops);
    benchmark_set(ops);
    
    std::cout << "\n=== Benchmark Complete ===" << std::endl;
    return 0;
}
