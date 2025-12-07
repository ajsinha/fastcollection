/**
 * FastCollection v1.0.0 - Cache Example
 * 
 * This example demonstrates using FastCollectionMap as a cache with TTL.
 * 
 * Copyright © 2025-2030, Ashutosh Sinha (ajsinha@gmail.com)
 * Patent Pending
 */
package examples;

import com.kuber.fastcollection.*;
import java.io.Serializable;
import java.util.*;

public class CacheExample {
    
    /**
     * Simple cache wrapper around FastCollectionMap
     */
    public static class Cache<K extends Serializable, V extends Serializable> {
        private final FastCollectionMap<K, V> store;
        private final int defaultTTL;
        
        public Cache(String path, int defaultTTLSeconds) {
            this.store = new FastCollectionMap<>(path, 32 * 1024 * 1024, true);
            this.defaultTTL = defaultTTLSeconds;
        }
        
        public void put(K key, V value) {
            store.put(key, value, defaultTTL);
        }
        
        public void put(K key, V value, int ttlSeconds) {
            store.put(key, value, ttlSeconds);
        }
        
        public V get(K key) {
            return store.get(key);
        }
        
        public V getOrDefault(K key, V defaultValue) {
            V value = store.get(key);
            return value != null ? value : defaultValue;
        }
        
        public boolean contains(K key) {
            return store.containsKey(key);
        }
        
        public void remove(K key) {
            store.remove(key);
        }
        
        public long getTTL(K key) {
            return store.getTTL(key);
        }
        
        public void refreshTTL(K key) {
            store.setTTL(key, defaultTTL);
        }
        
        public int size() {
            return store.size();
        }
        
        public int cleanup() {
            return store.removeExpired();
        }
        
        public void close() {
            store.close();
        }
    }
    
    public static void main(String[] args) throws InterruptedException {
        System.out.println("FastCollection v1.0.0 - Cache Example");
        System.out.println("=====================================\n");
        
        // Create cache with 10-second default TTL
        Cache<String, String> cache = new Cache<>("/tmp/cache_example.fc", 10);
        
        try {
            // Store user data
            System.out.println("Storing user data...");
            cache.put("user:1001", "John Doe");
            cache.put("user:1002", "Jane Smith");
            cache.put("user:1003", "Bob Johnson");
            
            // Store with custom TTL
            cache.put("session:abc123", "session_data", 30);  // 30 seconds
            cache.put("config:app", "config_value", -1);      // Never expires
            
            System.out.println("Cache size: " + cache.size());
            
            // Retrieve data
            System.out.println("\nRetrieving data:");
            System.out.println("  user:1001 = " + cache.get("user:1001"));
            System.out.println("  user:1002 = " + cache.get("user:1002"));
            System.out.println("  nonexistent = " + cache.getOrDefault("nonexistent", "DEFAULT"));
            
            // Check TTLs
            System.out.println("\nTTL values:");
            System.out.println("  user:1001 TTL = " + cache.getTTL("user:1001") + "s");
            System.out.println("  session:abc123 TTL = " + cache.getTTL("session:abc123") + "s");
            System.out.println("  config:app TTL = " + cache.getTTL("config:app") + " (infinite)");
            
            // Wait for some items to expire
            System.out.println("\nWaiting 12 seconds for user data to expire...");
            Thread.sleep(12000);
            
            // Check what's left
            System.out.println("\nAfter expiry:");
            System.out.println("  Cache size: " + cache.size());
            System.out.println("  user:1001 = " + cache.get("user:1001"));  // null (expired)
            System.out.println("  session:abc123 = " + cache.get("session:abc123"));  // still valid
            System.out.println("  config:app = " + cache.get("config:app"));  // never expires
            
            // Cleanup expired entries
            int removed = cache.cleanup();
            System.out.println("\nCleaned up " + removed + " expired entries");
            
            System.out.println("\nExample completed successfully!");
            
        } finally {
            cache.close();
        }
    }
}
