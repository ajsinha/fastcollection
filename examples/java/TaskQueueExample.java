/**
 * FastCollection v1.0.0 - Task Queue Example
 * 
 * Implements a persistent task queue with priorities using FastCollectionQueue.
 * 
 * Copyright © 2025-2030, Ashutosh Sinha (ajsinha@gmail.com)
 * Patent Pending
 */
package examples;

import com.kuber.fastcollection.*;
import java.io.Serializable;
import java.util.*;
import java.util.concurrent.*;

public class TaskQueueExample {
    
    public static class Task implements Serializable {
        private static final long serialVersionUID = 1L;
        
        public String id;
        public String type;
        public String payload;
        public int priority;  // 0 = highest
        public long createdAt;
        public int retryCount;
        public int maxRetries;
        
        public Task(String id, String type, String payload, int priority) {
            this.id = id;
            this.type = type;
            this.payload = payload;
            this.priority = priority;
            this.createdAt = System.currentTimeMillis();
            this.retryCount = 0;
            this.maxRetries = 3;
        }
        
        @Override
        public String toString() {
            return String.format("Task{id=%s, type=%s, priority=%d, retries=%d/%d}",
                id, type, priority, retryCount, maxRetries);
        }
    }
    
    public static class TaskQueue {
        private final FastCollectionQueue<Task> queue;
        private final FastCollectionQueue<Task> deadLetterQueue;
        private final int taskTTL;
        
        public TaskQueue(String basePath, int taskTTLSeconds) {
            this.queue = new FastCollectionQueue<>(basePath + "/tasks.fc", 64 * 1024 * 1024, true);
            this.deadLetterQueue = new FastCollectionQueue<>(basePath + "/dlq.fc", 16 * 1024 * 1024, true);
            this.taskTTL = taskTTLSeconds;
        }
        
        public void submit(Task task) {
            if (task.priority == 0) {
                // High priority - add to front
                queue.offerFirst(task, taskTTL);
            } else {
                queue.offer(task, taskTTL);
            }
            System.out.println("Submitted: " + task);
        }
        
        public Task poll() {
            return queue.poll();
        }
        
        public Task peek() {
            return queue.peek();
        }
        
        public void requeue(Task task) {
            task.retryCount++;
            if (task.retryCount >= task.maxRetries) {
                System.out.println("Moving to DLQ: " + task);
                deadLetterQueue.offer(task, -1);  // Keep forever in DLQ
            } else {
                System.out.println("Requeuing: " + task);
                queue.offer(task, taskTTL);
            }
        }
        
        public int size() {
            return queue.size();
        }
        
        public int deadLetterSize() {
            return deadLetterQueue.size();
        }
        
        public void close() {
            queue.close();
            deadLetterQueue.close();
        }
    }
    
    public static void main(String[] args) throws InterruptedException {
        System.out.println("FastCollection v1.0.0 - Task Queue Example");
        System.out.println("==========================================\n");
        
        TaskQueue taskQueue = new TaskQueue("/tmp/taskqueue", 3600);
        
        try {
            // Submit various tasks
            System.out.println("Submitting tasks...\n");
            
            taskQueue.submit(new Task("t1", "EMAIL", "Send welcome email to user@example.com", 2));
            taskQueue.submit(new Task("t2", "REPORT", "Generate daily sales report", 3));
            taskQueue.submit(new Task("t3", "ALERT", "Critical system alert!", 0));  // High priority
            taskQueue.submit(new Task("t4", "BACKUP", "Backup user database", 5));
            taskQueue.submit(new Task("t5", "NOTIFY", "Push notification to mobile", 1));
            
            System.out.println("\nQueue size: " + taskQueue.size());
            System.out.println("Next task: " + taskQueue.peek());
            
            // Process tasks
            System.out.println("\n--- Processing Tasks ---\n");
            
            Random random = new Random();
            Task task;
            
            while ((task = taskQueue.poll()) != null) {
                System.out.println("Processing: " + task);
                
                // Simulate random failures (30% chance)
                if (random.nextInt(10) < 3) {
                    System.out.println("  FAILED! Requeuing...");
                    taskQueue.requeue(task);
                } else {
                    System.out.println("  SUCCESS!");
                }
                
                Thread.sleep(100);  // Simulate work
            }
            
            System.out.println("\n--- Summary ---");
            System.out.println("Queue size: " + taskQueue.size());
            System.out.println("Dead letter queue size: " + taskQueue.deadLetterSize());
            
        } finally {
            taskQueue.close();
        }
    }
}
