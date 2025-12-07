#!/usr/bin/env python3
"""
FastCollection v1.0.0 - Cache Example (Python)

This example demonstrates using FastMap as a cache with TTL.

Usage: python cache_example.py

Copyright © 2025-2030, Ashutosh Sinha (ajsinha@gmail.com)
Patent Pending
"""

from fastcollection import FastMap
import json
import time


class Cache:
    """Simple cache wrapper around FastMap"""
    
    def __init__(self, path: str, default_ttl: int = 300):
        self.store = FastMap(path)
        self.default_ttl = default_ttl
    
    def put(self, key: str, value, ttl: int = None):
        """Store a value with optional TTL"""
        ttl = ttl if ttl is not None else self.default_ttl
        data = json.dumps(value).encode()
        self.store.put(key.encode(), data, ttl=ttl)
    
    def get(self, key: str, default=None):
        """Retrieve a value"""
        data = self.store.get(key.encode())
        if data is None:
            return default
        return json.loads(data.decode())
    
    def contains(self, key: str) -> bool:
        """Check if key exists"""
        return self.store.contains_key(key.encode())
    
    def delete(self, key: str):
        """Delete a key"""
        self.store.remove(key.encode())
    
    def get_ttl(self, key: str) -> int:
        """Get remaining TTL in seconds"""
        return self.store.get_ttl(key.encode())
    
    def refresh_ttl(self, key: str, ttl: int = None):
        """Refresh TTL for a key"""
        ttl = ttl if ttl is not None else self.default_ttl
        self.store.set_ttl(key.encode(), ttl)
    
    def size(self) -> int:
        """Get number of items"""
        return self.store.size()
    
    def cleanup(self) -> int:
        """Remove expired items"""
        return self.store.remove_expired()
    
    def close(self):
        """Close the cache"""
        self.store.close()
    
    def __enter__(self):
        return self
    
    def __exit__(self, *args):
        self.close()


def main():
    print("FastCollection v1.0.0 - Cache Example (Python)")
    print("=" * 50)
    print()
    
    # Create cache with 10-second default TTL
    with Cache("/tmp/cache_example_py.fc", default_ttl=10) as cache:
        # Store user data
        print("Storing user data...")
        cache.put("user:1001", {"name": "John Doe", "email": "john@example.com"})
        cache.put("user:1002", {"name": "Jane Smith", "email": "jane@example.com"})
        cache.put("user:1003", {"name": "Bob Johnson", "email": "bob@example.com"})
        
        # Store with custom TTL
        cache.put("session:abc123", {"token": "xyz", "user_id": "1001"}, ttl=30)
        cache.put("config:app", {"debug": True, "version": "1.0.0"}, ttl=-1)  # Never expires
        
        print(f"Cache size: {cache.size()}")
        
        # Retrieve data
        print("\nRetrieving data:")
        user1 = cache.get("user:1001")
        print(f"  user:1001 = {user1}")
        
        user2 = cache.get("user:1002")
        print(f"  user:1002 = {user2}")
        
        default = cache.get("nonexistent", {"default": True})
        print(f"  nonexistent = {default}")
        
        # Check TTLs
        print("\nTTL values:")
        print(f"  user:1001 TTL = {cache.get_ttl('user:1001')}s")
        print(f"  session:abc123 TTL = {cache.get_ttl('session:abc123')}s")
        print(f"  config:app TTL = {cache.get_ttl('config:app')} (infinite)")
        
        # Wait for some items to expire
        print("\nWaiting 12 seconds for user data to expire...")
        time.sleep(12)
        
        # Check what's left
        print("\nAfter expiry:")
        print(f"  Cache size: {cache.size()}")
        print(f"  user:1001 = {cache.get('user:1001')}")  # None (expired)
        print(f"  session:abc123 = {cache.get('session:abc123')}")  # still valid
        print(f"  config:app = {cache.get('config:app')}")  # never expires
        
        # Cleanup expired entries
        removed = cache.cleanup()
        print(f"\nCleaned up {removed} expired entries")
        
        print("\nExample completed successfully!")


if __name__ == "__main__":
    main()
