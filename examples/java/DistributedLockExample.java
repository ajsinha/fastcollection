/**
 * FastCollection v1.0.0 - Distributed Lock Example
 * 
 * Implements a simple distributed lock using FastCollectionMap.
 * 
 * Copyright © 2025-2030, Ashutosh Sinha (ajsinha@gmail.com)
 * Patent Pending
 */
package examples;

import com.kuber.fastcollection.*;
import java.io.Serializable;
import java.util.UUID;
import java.util.concurrent.*;

public class DistributedLockExample {
    
    public static class LockInfo implements Serializable {
        private static final long serialVersionUID = 1L;
        
        public String lockId;
        public String owner;
        public long acquiredAt;
        public long expiresAt;
        
        public LockInfo(String lockId, String owner, int ttlSeconds) {
            this.lockId = lockId;
            this.owner = owner;
            this.acquiredAt = System.currentTimeMillis();
            this.expiresAt = this.acquiredAt + (ttlSeconds * 1000L);
        }
        
        public boolean isExpired() {
            return System.currentTimeMillis() > expiresAt;
        }
    }
    
    public static class DistributedLock {
        private final FastCollectionMap<String, LockInfo> lockStore;
        private final String nodeId;
        private final int defaultTTL;
        
        public DistributedLock(String path, int defaultTTLSeconds) {
            this.lockStore = new FastCollectionMap<>(path, 16 * 1024 * 1024, true);
            this.nodeId = "node_" + UUID.randomUUID().toString().substring(0, 8);
            this.defaultTTL = defaultTTLSeconds;
        }
        
        public boolean tryLock(String resourceId) {
            return tryLock(resourceId, defaultTTL);
        }
        
        public synchronized boolean tryLock(String resourceId, int ttlSeconds) {
            LockInfo existing = lockStore.get(resourceId);
            
            // Check if lock exists and is still valid
            if (existing != null && !existing.isExpired()) {
                // Already locked by someone
                return false;
            }
            
            // Acquire lock
            LockInfo lock = new LockInfo(resourceId, nodeId, ttlSeconds);
            lockStore.put(resourceId, lock, ttlSeconds);
            return true;
        }
        
        public boolean lock(String resourceId, long timeoutMs) throws InterruptedException {
            long deadline = System.currentTimeMillis() + timeoutMs;
            
            while (System.currentTimeMillis() < deadline) {
                if (tryLock(resourceId)) {
                    return true;
                }
                Thread.sleep(50);  // Retry interval
            }
            
            return false;  // Timeout
        }
        
        public synchronized boolean unlock(String resourceId) {
            LockInfo lock = lockStore.get(resourceId);
            
            if (lock == null) {
                return false;  // No lock
            }
            
            if (!lock.owner.equals(nodeId)) {
                return false;  // Not our lock
            }
            
            lockStore.remove(resourceId);
            return true;
        }
        
        public synchronized boolean extendLock(String resourceId, int additionalSeconds) {
            LockInfo lock = lockStore.get(resourceId);
            
            if (lock == null || !lock.owner.equals(nodeId)) {
                return false;
            }
            
            long currentTTL = lockStore.getTTL(resourceId);
            lockStore.setTTL(resourceId, (int) currentTTL + additionalSeconds);
            lock.expiresAt += additionalSeconds * 1000L;
            lockStore.put(resourceId, lock);
            
            return true;
        }
        
        public boolean isLocked(String resourceId) {
            LockInfo lock = lockStore.get(resourceId);
            return lock != null && !lock.isExpired();
        }
        
        public String getLockOwner(String resourceId) {
            LockInfo lock = lockStore.get(resourceId);
            return lock != null ? lock.owner : null;
        }
        
        public String getNodeId() {
            return nodeId;
        }
        
        public void close() {
            lockStore.close();
        }
    }
    
    public static void main(String[] args) throws Exception {
        System.out.println("FastCollection v1.0.0 - Distributed Lock Example");
        System.out.println("================================================\n");
        
        // Create lock manager (locks expire after 30 seconds)
        DistributedLock lockManager = new DistributedLock("/tmp/locks.fc", 30);
        
        try {
            System.out.println("Node ID: " + lockManager.getNodeId() + "\n");
            
            String resourceId = "critical_section_1";
            
            // Try to acquire lock
            System.out.println("Attempting to acquire lock on: " + resourceId);
            
            if (lockManager.tryLock(resourceId)) {
                System.out.println("✓ Lock acquired successfully!\n");
                
                // Simulate critical section
                System.out.println("Executing critical section...");
                
                for (int i = 1; i <= 5; i++) {
                    System.out.println("  Step " + i + "/5 - Working...");
                    Thread.sleep(500);
                }
                
                // Release lock
                if (lockManager.unlock(resourceId)) {
                    System.out.println("\n✓ Lock released successfully!");
                }
            } else {
                System.out.println("✗ Failed to acquire lock - resource is busy");
            }
            
            // Demonstrate concurrent access
            System.out.println("\n=== Concurrent Access Demo ===\n");
            
            ExecutorService executor = Executors.newFixedThreadPool(3);
            CountDownLatch latch = new CountDownLatch(3);
            
            for (int i = 1; i <= 3; i++) {
                final int workerId = i;
                executor.submit(() -> {
                    try {
                        String worker = "Worker-" + workerId;
                        System.out.println(worker + ": Attempting to acquire lock...");
                        
                        if (lockManager.lock("shared_resource", 5000)) {
                            System.out.println(worker + ": ✓ Got lock! Working...");
                            Thread.sleep(1000);
                            lockManager.unlock("shared_resource");
                            System.out.println(worker + ": Done, lock released.");
                        } else {
                            System.out.println(worker + ": ✗ Timeout waiting for lock");
                        }
                    } catch (InterruptedException e) {
                        Thread.currentThread().interrupt();
                    } finally {
                        latch.countDown();
                    }
                });
            }
            
            latch.await();
            executor.shutdown();
            
            System.out.println("\nAll workers completed.");
            
        } finally {
            lockManager.close();
        }
    }
}
