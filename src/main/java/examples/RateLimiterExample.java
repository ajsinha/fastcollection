/**
 * FastCollection v1.0.0 - Rate Limiter Example
 * 
 * Implements a sliding window rate limiter using FastCollectionMap.
 * 
 * Copyright © 2025-2030, Ashutosh Sinha (ajsinha@gmail.com)
 * Patent Pending
 */
package examples;

import com.kuber.fastcollection.*;
import java.io.Serializable;

public class RateLimiterExample {
    
    public static class RateLimitEntry implements Serializable {
        private static final long serialVersionUID = 1L;
        public int count;
        public long windowStart;
        
        public RateLimitEntry() {
            this.count = 1;
            this.windowStart = System.currentTimeMillis();
        }
        
        public void increment() {
            this.count++;
        }
    }
    
    public static class RateLimiter {
        private final FastCollectionMap<String, RateLimitEntry> store;
        private final int maxRequests;
        private final int windowSeconds;
        
        public RateLimiter(String path, int maxRequests, int windowSeconds) {
            this.store = new FastCollectionMap<>(path, 32 * 1024 * 1024, true);
            this.maxRequests = maxRequests;
            this.windowSeconds = windowSeconds;
        }
        
        public synchronized boolean allowRequest(String clientId) {
            RateLimitEntry entry = store.get(clientId);
            
            if (entry == null) {
                store.put(clientId, new RateLimitEntry(), windowSeconds);
                return true;
            }
            
            if (entry.count >= maxRequests) {
                return false;
            }
            
            entry.increment();
            store.put(clientId, entry, windowSeconds);
            return true;
        }
        
        public int getRemainingRequests(String clientId) {
            RateLimitEntry entry = store.get(clientId);
            if (entry == null) {
                return maxRequests;
            }
            return Math.max(0, maxRequests - entry.count);
        }
        
        public long getResetTime(String clientId) {
            return store.getTTL(clientId);
        }
        
        public void close() {
            store.close();
        }
    }
    
    public static void main(String[] args) {
        System.out.println("FastCollection v1.0.0 - Rate Limiter Example");
        System.out.println("============================================\n");
        
        // 10 requests per 60 seconds
        RateLimiter limiter = new RateLimiter("/tmp/ratelimiter.fc", 10, 60);
        
        try {
            String clientId = "api_client_001";
            
            // Simulate API requests
            System.out.println("Simulating 15 API requests...\n");
            
            for (int i = 1; i <= 15; i++) {
                boolean allowed = limiter.allowRequest(clientId);
                int remaining = limiter.getRemainingRequests(clientId);
                
                System.out.printf("Request %2d: %s (remaining: %d)\n",
                    i,
                    allowed ? "✓ ALLOWED" : "✗ DENIED ",
                    remaining);
            }
            
            System.out.println("\nReset in: " + limiter.getResetTime(clientId) + " seconds");
            
        } finally {
            limiter.close();
        }
    }
}
