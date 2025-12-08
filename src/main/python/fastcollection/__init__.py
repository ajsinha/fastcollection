"""
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
    - ttl=0: Element expires immediately (testing)
    - ttl>0: Element expires after N seconds

Example:
    >>> from fastcollection import FastList, FastMap, TTL_INFINITE
    >>> 
    >>> # List operations
    >>> lst = FastList("/tmp/mylist.fc")
    >>> lst.add(b"hello", ttl=300)  # 5-minute TTL
    >>> lst.add(b"world")           # Never expires
    >>> print(lst.get(0))
    >>> lst.close()
    >>>
    >>> # Map operations  
    >>> m = FastMap("/tmp/mymap.fc")
    >>> m.put(b"key", b"value", ttl=60)
    >>> print(m.get(b"key"))
    >>> m.close()
"""

try:
    # Try to import from the native extension module
    from fastcollection._native import (
        FastList,
        FastSet,
        FastMap,
        FastQueue,
        FastStack,
        FastCollectionException,
        TTL_INFINITE,
    )
except ImportError:
    # Native module not built yet
    import warnings
    warnings.warn(
        "FastCollection native module not found. "
        "Build with: pip install . or python setup.py build_ext --inplace"
    )
    
    # Provide stub for IDE support
    TTL_INFINITE = -1
    
    class FastCollectionException(Exception):
        pass
    
    class FastList:
        def __init__(self, file_path, initial_size=67108864, create_new=False):
            raise NotImplementedError("Native module not built")
    
    class FastSet:
        def __init__(self, file_path, initial_size=67108864, create_new=False, bucket_count=16384):
            raise NotImplementedError("Native module not built")
    
    class FastMap:
        def __init__(self, file_path, initial_size=67108864, create_new=False, bucket_count=16384):
            raise NotImplementedError("Native module not built")
    
    class FastQueue:
        def __init__(self, file_path, initial_size=67108864, create_new=False):
            raise NotImplementedError("Native module not built")
    
    class FastStack:
        def __init__(self, file_path, initial_size=67108864, create_new=False):
            raise NotImplementedError("Native module not built")

__version__ = "1.0.0"
__author__ = "Ashutosh Sinha"
__email__ = "ajsinha@gmail.com"
__all__ = [
    "FastList",
    "FastSet", 
    "FastMap",
    "FastQueue",
    "FastStack",
    "FastCollectionException",
    "TTL_INFINITE",
]
