/**
 * Copyright © 2025-2030, All Rights Reserved
 * Ashutosh Sinha | Email: ajsinha@gmail.com
 * 
 * Patent Pending
 * 
 * @file pyfastcollection.cpp
 * @brief Python bindings for FastCollection using pybind11
 */

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/functional.h>

#include "fc_list.h"
#include "fc_set.h"
#include "fc_map.h"
#include "fc_queue.h"
#include "fc_stack.h"

namespace py = pybind11;
using namespace fastcollection;

// Helper to convert Python bytes to vector
std::vector<uint8_t> bytes_to_vector(const py::bytes& b) {
    std::string s = b;
    return std::vector<uint8_t>(s.begin(), s.end());
}

// Helper to convert vector to Python bytes
py::bytes vector_to_bytes(const std::vector<uint8_t>& v) {
    return py::bytes(reinterpret_cast<const char*>(v.data()), v.size());
}

PYBIND11_MODULE(_native, m) {
    m.doc() = R"pbdoc(
        FastCollection - Ultra High-Performance Memory-Mapped Collections with TTL
        
        Copyright © 2025-2030, Ashutosh Sinha (ajsinha@gmail.com)
        Patent Pending
        
        Collections:
            - FastList: Doubly-linked list with O(1) head/tail operations
            - FastSet: Hash set with O(1) lookups
            - FastMap: Key-value store with atomic operations
            - FastQueue: FIFO queue with deque operations
            - FastStack: LIFO stack with lock-free push/pop
        
        TTL Support:
            - ttl=-1 (default): Element never expires
            - ttl=0: Element expires immediately
            - ttl>0: Element expires after N seconds
        
        Example:
            >>> from fastcollection import FastList
            >>> lst = FastList("/tmp/mylist.fc")
            >>> lst.add(b"hello", ttl=300)  # 5-minute TTL
            >>> lst.add(b"world")           # Never expires
            >>> data = lst.get(0)
            >>> lst.close()
    )pbdoc";
    
    // TTL constant
    m.attr("TTL_INFINITE") = TTL_INFINITE;
    
    // Exception
    py::register_exception<FastCollectionException>(m, "FastCollectionException");
    
    // ========================================================================
    // FastList
    // ========================================================================
    py::class_<FastList>(m, "FastList", R"pbdoc(
        Ultra high-performance memory-mapped list with TTL support.
        
        Args:
            file_path: Path to memory-mapped file
            initial_size: Initial file size in bytes (default: 64MB)
            create_new: If True, truncate existing file
        
        Example:
            >>> lst = FastList("/tmp/mylist.fc")
            >>> lst.add(b"data", ttl=300)  # 5-minute TTL
            >>> lst.add(b"permanent")       # Never expires (ttl=-1)
            >>> data = lst.get(0)
            >>> remaining = lst.get_ttl(0)  # -1 if infinite
    )pbdoc")
        .def(py::init<const std::string&, size_t, bool>(),
             py::arg("file_path"),
             py::arg("initial_size") = DEFAULT_INITIAL_SIZE,
             py::arg("create_new") = false)
        
        .def("add", [](FastList& self, const py::bytes& data, int32_t ttl) {
            auto vec = bytes_to_vector(data);
            return self.add(vec.data(), vec.size(), ttl);
        }, py::arg("data"), py::arg("ttl") = TTL_INFINITE,
           "Add element to end. ttl=-1 means never expires.")
        
        .def("add_at", [](FastList& self, size_t index, const py::bytes& data, int32_t ttl) {
            auto vec = bytes_to_vector(data);
            return self.add(index, vec.data(), vec.size(), ttl);
        }, py::arg("index"), py::arg("data"), py::arg("ttl") = TTL_INFINITE)
        
        .def("add_first", [](FastList& self, const py::bytes& data, int32_t ttl) {
            auto vec = bytes_to_vector(data);
            return self.addFirst(vec.data(), vec.size(), ttl);
        }, py::arg("data"), py::arg("ttl") = TTL_INFINITE)
        
        .def("get", [](FastList& self, size_t index) -> py::object {
            std::vector<uint8_t> result;
            if (self.get(index, result)) {
                return vector_to_bytes(result);
            }
            return py::none();
        }, py::arg("index"))
        
        .def("get_first", [](FastList& self) -> py::object {
            std::vector<uint8_t> result;
            if (self.getFirst(result)) return vector_to_bytes(result);
            return py::none();
        })
        
        .def("get_last", [](FastList& self) -> py::object {
            std::vector<uint8_t> result;
            if (self.getLast(result)) return vector_to_bytes(result);
            return py::none();
        })
        
        .def("set", [](FastList& self, size_t index, const py::bytes& data, int32_t ttl) {
            auto vec = bytes_to_vector(data);
            return self.set(index, vec.data(), vec.size(), ttl);
        }, py::arg("index"), py::arg("data"), py::arg("ttl") = TTL_INFINITE)
        
        .def("remove", [](FastList& self, size_t index) -> py::object {
            std::vector<uint8_t> result;
            if (self.remove(index, &result)) return vector_to_bytes(result);
            return py::none();
        }, py::arg("index"))
        
        .def("remove_first", [](FastList& self) -> py::object {
            std::vector<uint8_t> result;
            if (self.removeFirst(&result)) return vector_to_bytes(result);
            return py::none();
        })
        
        .def("remove_last", [](FastList& self) -> py::object {
            std::vector<uint8_t> result;
            if (self.removeLast(&result)) return vector_to_bytes(result);
            return py::none();
        })
        
        .def("contains", [](FastList& self, const py::bytes& data) {
            auto vec = bytes_to_vector(data);
            return self.contains(vec.data(), vec.size());
        }, py::arg("data"))
        
        .def("index_of", [](FastList& self, const py::bytes& data) {
            auto vec = bytes_to_vector(data);
            return self.indexOf(vec.data(), vec.size());
        }, py::arg("data"))
        
        .def("get_ttl", &FastList::getTTL, py::arg("index"),
             "Get remaining TTL. Returns -1 if infinite, 0 if expired.")
        .def("set_ttl", &FastList::setTTL, py::arg("index"), py::arg("ttl_seconds"))
        .def("remove_expired", &FastList::removeExpired)
        .def("clear", &FastList::clear)
        .def("size", &FastList::size)
        .def("is_empty", &FastList::isEmpty)
        .def("flush", &FastList::flush)
        .def("filename", &FastList::filename)
        .def("__len__", &FastList::size)
        .def("__bool__", [](FastList& self) { return !self.isEmpty(); })
        .def("close", [](FastList& self) { self.flush(); });
    
    // ========================================================================
    // FastSet
    // ========================================================================
    py::class_<FastSet>(m, "FastSet", "Ultra high-performance memory-mapped set with TTL.")
        .def(py::init<const std::string&, size_t, bool, uint32_t>(),
             py::arg("file_path"),
             py::arg("initial_size") = DEFAULT_INITIAL_SIZE,
             py::arg("create_new") = false,
             py::arg("bucket_count") = HashTableHeader::DEFAULT_BUCKET_COUNT)
        
        .def("add", [](FastSet& self, const py::bytes& data, int32_t ttl) {
            auto vec = bytes_to_vector(data);
            return self.add(vec.data(), vec.size(), ttl);
        }, py::arg("data"), py::arg("ttl") = TTL_INFINITE)
        
        .def("remove", [](FastSet& self, const py::bytes& data) {
            auto vec = bytes_to_vector(data);
            return self.remove(vec.data(), vec.size());
        }, py::arg("data"))
        
        .def("contains", [](FastSet& self, const py::bytes& data) {
            auto vec = bytes_to_vector(data);
            return self.contains(vec.data(), vec.size());
        }, py::arg("data"))
        
        .def("get_ttl", [](FastSet& self, const py::bytes& data) {
            auto vec = bytes_to_vector(data);
            return self.getTTL(vec.data(), vec.size());
        }, py::arg("data"))
        
        .def("set_ttl", [](FastSet& self, const py::bytes& data, int32_t ttl) {
            auto vec = bytes_to_vector(data);
            return self.setTTL(vec.data(), vec.size(), ttl);
        }, py::arg("data"), py::arg("ttl_seconds"))
        
        .def("remove_expired", &FastSet::removeExpired)
        .def("clear", &FastSet::clear)
        .def("size", &FastSet::size)
        .def("is_empty", &FastSet::isEmpty)
        .def("flush", &FastSet::flush)
        .def("__len__", &FastSet::size)
        .def("__contains__", [](FastSet& self, const py::bytes& data) {
            auto vec = bytes_to_vector(data);
            return self.contains(vec.data(), vec.size());
        })
        .def("close", [](FastSet& self) { self.flush(); });
    
    // ========================================================================
    // FastMap
    // ========================================================================
    py::class_<FastMap>(m, "FastMap", "Ultra high-performance memory-mapped map with TTL.")
        .def(py::init<const std::string&, size_t, bool, uint32_t>(),
             py::arg("file_path"),
             py::arg("initial_size") = DEFAULT_INITIAL_SIZE,
             py::arg("create_new") = false,
             py::arg("bucket_count") = HashTableHeader::DEFAULT_BUCKET_COUNT)
        
        .def("put", [](FastMap& self, const py::bytes& key, const py::bytes& value, int32_t ttl) {
            auto k = bytes_to_vector(key);
            auto v = bytes_to_vector(value);
            return self.put(k.data(), k.size(), v.data(), v.size(), ttl);
        }, py::arg("key"), py::arg("value"), py::arg("ttl") = TTL_INFINITE)
        
        .def("put_if_absent", [](FastMap& self, const py::bytes& key, const py::bytes& value, int32_t ttl) {
            auto k = bytes_to_vector(key);
            auto v = bytes_to_vector(value);
            return self.putIfAbsent(k.data(), k.size(), v.data(), v.size(), ttl);
        }, py::arg("key"), py::arg("value"), py::arg("ttl") = TTL_INFINITE)
        
        .def("get", [](FastMap& self, const py::bytes& key) -> py::object {
            auto k = bytes_to_vector(key);
            std::vector<uint8_t> result;
            if (self.get(k.data(), k.size(), result)) {
                return vector_to_bytes(result);
            }
            return py::none();
        }, py::arg("key"))
        
        .def("remove", [](FastMap& self, const py::bytes& key) {
            auto k = bytes_to_vector(key);
            return self.remove(k.data(), k.size());
        }, py::arg("key"))
        
        .def("contains_key", [](FastMap& self, const py::bytes& key) {
            auto k = bytes_to_vector(key);
            return self.containsKey(k.data(), k.size());
        }, py::arg("key"))
        
        .def("get_ttl", [](FastMap& self, const py::bytes& key) {
            auto k = bytes_to_vector(key);
            return self.getTTL(k.data(), k.size());
        }, py::arg("key"))
        
        .def("set_ttl", [](FastMap& self, const py::bytes& key, int32_t ttl) {
            auto k = bytes_to_vector(key);
            return self.setTTL(k.data(), k.size(), ttl);
        }, py::arg("key"), py::arg("ttl_seconds"))
        
        .def("remove_expired", &FastMap::removeExpired)
        .def("clear", &FastMap::clear)
        .def("size", &FastMap::size)
        .def("is_empty", &FastMap::isEmpty)
        .def("flush", &FastMap::flush)
        .def("__len__", &FastMap::size)
        .def("__getitem__", [](FastMap& self, const py::bytes& key) -> py::object {
            auto k = bytes_to_vector(key);
            std::vector<uint8_t> result;
            if (self.get(k.data(), k.size(), result)) {
                return vector_to_bytes(result);
            }
            throw py::key_error("Key not found");
        })
        .def("__setitem__", [](FastMap& self, const py::bytes& key, const py::bytes& value) {
            auto k = bytes_to_vector(key);
            auto v = bytes_to_vector(value);
            self.put(k.data(), k.size(), v.data(), v.size(), TTL_INFINITE);
        })
        .def("__contains__", [](FastMap& self, const py::bytes& key) {
            auto k = bytes_to_vector(key);
            return self.containsKey(k.data(), k.size());
        })
        .def("close", [](FastMap& self) { self.flush(); });
    
    // ========================================================================
    // FastQueue
    // ========================================================================
    py::class_<FastQueue>(m, "FastQueue", "Ultra high-performance memory-mapped queue with TTL.")
        .def(py::init<const std::string&, size_t, bool>(),
             py::arg("file_path"),
             py::arg("initial_size") = DEFAULT_INITIAL_SIZE,
             py::arg("create_new") = false)
        
        .def("offer", [](FastQueue& self, const py::bytes& data, int32_t ttl) {
            auto vec = bytes_to_vector(data);
            return self.offer(vec.data(), vec.size(), ttl);
        }, py::arg("data"), py::arg("ttl") = TTL_INFINITE)
        
        .def("offer_first", [](FastQueue& self, const py::bytes& data, int32_t ttl) {
            auto vec = bytes_to_vector(data);
            return self.offerFirst(vec.data(), vec.size(), ttl);
        }, py::arg("data"), py::arg("ttl") = TTL_INFINITE)
        
        .def("poll", [](FastQueue& self) -> py::object {
            std::vector<uint8_t> result;
            if (self.poll(result)) return vector_to_bytes(result);
            return py::none();
        })
        
        .def("poll_last", [](FastQueue& self) -> py::object {
            std::vector<uint8_t> result;
            if (self.pollLast(result)) return vector_to_bytes(result);
            return py::none();
        })
        
        .def("peek", [](FastQueue& self) -> py::object {
            std::vector<uint8_t> result;
            if (self.peek(result)) return vector_to_bytes(result);
            return py::none();
        })
        
        .def("peek_ttl", &FastQueue::peekTTL)
        .def("remove_expired", &FastQueue::removeExpired)
        .def("clear", &FastQueue::clear)
        .def("size", &FastQueue::size)
        .def("is_empty", &FastQueue::isEmpty)
        .def("flush", &FastQueue::flush)
        .def("__len__", &FastQueue::size)
        .def("close", [](FastQueue& self) { self.flush(); });
    
    // ========================================================================
    // FastStack
    // ========================================================================
    py::class_<FastStack>(m, "FastStack", "Ultra high-performance memory-mapped stack with TTL.")
        .def(py::init<const std::string&, size_t, bool>(),
             py::arg("file_path"),
             py::arg("initial_size") = DEFAULT_INITIAL_SIZE,
             py::arg("create_new") = false)
        
        .def("push", [](FastStack& self, const py::bytes& data, int32_t ttl) {
            auto vec = bytes_to_vector(data);
            return self.push(vec.data(), vec.size(), ttl);
        }, py::arg("data"), py::arg("ttl") = TTL_INFINITE)
        
        .def("pop", [](FastStack& self) -> py::object {
            std::vector<uint8_t> result;
            if (self.pop(result)) return vector_to_bytes(result);
            return py::none();
        })
        
        .def("peek", [](FastStack& self) -> py::object {
            std::vector<uint8_t> result;
            if (self.peek(result)) return vector_to_bytes(result);
            return py::none();
        })
        
        .def("search", [](FastStack& self, const py::bytes& data) {
            auto vec = bytes_to_vector(data);
            return self.search(vec.data(), vec.size());
        }, py::arg("data"))
        
        .def("peek_ttl", &FastStack::peekTTL)
        .def("remove_expired", &FastStack::removeExpired)
        .def("clear", &FastStack::clear)
        .def("size", &FastStack::size)
        .def("is_empty", &FastStack::isEmpty)
        .def("flush", &FastStack::flush)
        .def("__len__", &FastStack::size)
        .def("close", [](FastStack& self) { self.flush(); });
}
