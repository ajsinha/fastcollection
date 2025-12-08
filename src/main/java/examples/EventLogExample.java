/**
 * FastCollection v1.0.0 - Event Log Example
 * 
 * Implements a persistent event log with retention using FastCollectionList.
 * 
 * Copyright © 2025-2030, Ashutosh Sinha (ajsinha@gmail.com)
 * Patent Pending
 */
package examples;

import com.kuber.fastcollection.*;
import java.io.Serializable;
import java.time.*;
import java.time.format.DateTimeFormatter;
import java.util.*;

public class EventLogExample {
    
    public enum LogLevel { DEBUG, INFO, WARN, ERROR, FATAL }
    
    public static class LogEvent implements Serializable {
        private static final long serialVersionUID = 1L;
        
        public LogLevel level;
        public String message;
        public String source;
        public String threadName;
        public long timestamp;
        public Map<String, String> metadata;
        
        public LogEvent(LogLevel level, String source, String message) {
            this.level = level;
            this.source = source;
            this.message = message;
            this.threadName = Thread.currentThread().getName();
            this.timestamp = System.currentTimeMillis();
            this.metadata = new HashMap<>();
        }
        
        public LogEvent withMetadata(String key, String value) {
            this.metadata.put(key, value);
            return this;
        }
        
        @Override
        public String toString() {
            String time = Instant.ofEpochMilli(timestamp)
                .atZone(ZoneId.systemDefault())
                .format(DateTimeFormatter.ofPattern("yyyy-MM-dd HH:mm:ss.SSS"));
            
            StringBuilder sb = new StringBuilder();
            sb.append(String.format("[%s] %-5s [%s] %s: %s",
                time, level, threadName, source, message));
            
            if (!metadata.isEmpty()) {
                sb.append(" | ").append(metadata);
            }
            
            return sb.toString();
        }
    }
    
    public static class EventLogger {
        private final FastCollectionList<LogEvent> logs;
        private final int retentionSeconds;
        private final String name;
        
        public EventLogger(String name, String path, int retentionDays) {
            this.name = name;
            this.logs = new FastCollectionList<>(path, LogEvent.class, 64 * 1024 * 1024, true);
            this.retentionSeconds = retentionDays * 24 * 60 * 60;
        }
        
        private void log(LogLevel level, String source, String message) {
            LogEvent event = new LogEvent(level, source, message);
            logs.add(event, retentionSeconds);
        }
        
        public void debug(String source, String message) {
            log(LogLevel.DEBUG, source, message);
        }
        
        public void info(String source, String message) {
            log(LogLevel.INFO, source, message);
        }
        
        public void warn(String source, String message) {
            log(LogLevel.WARN, source, message);
        }
        
        public void error(String source, String message) {
            log(LogLevel.ERROR, source, message);
        }
        
        public void fatal(String source, String message) {
            log(LogLevel.FATAL, source, message);
        }
        
        public void logWithMetadata(LogLevel level, String source, String message, 
                                    Map<String, String> metadata) {
            LogEvent event = new LogEvent(level, source, message);
            event.metadata.putAll(metadata);
            logs.add(event, retentionSeconds);
        }
        
        public List<LogEvent> getLast(int count) {
            List<LogEvent> result = new ArrayList<>();
            int size = logs.size();
            int start = Math.max(0, size - count);
            
            for (int i = start; i < size; i++) {
                LogEvent event = logs.get(i);
                if (event != null) {
                    result.add(event);
                }
            }
            return result;
        }
        
        public List<LogEvent> getByLevel(LogLevel level, int maxCount) {
            List<LogEvent> result = new ArrayList<>();
            int size = logs.size();
            
            for (int i = size - 1; i >= 0 && result.size() < maxCount; i--) {
                LogEvent event = logs.get(i);
                if (event != null && event.level == level) {
                    result.add(event);
                }
            }
            
            Collections.reverse(result);
            return result;
        }
        
        public int cleanup() {
            return logs.removeExpired();
        }
        
        public int size() {
            return logs.size();
        }
        
        public void close() {
            logs.close();
        }
    }
    
    public static void main(String[] args) {
        System.out.println("FastCollection v1.0.0 - Event Log Example");
        System.out.println("=========================================\n");
        
        // Create logger with 7-day retention
        EventLogger logger = new EventLogger("AppLogger", "/tmp/eventlog.fc", 7);
        
        try {
            // Simulate application events
            System.out.println("Logging events...\n");
            
            logger.info("Application", "Application started");
            logger.info("Database", "Connected to PostgreSQL at localhost:5432");
            logger.debug("Cache", "Initialized cache with 1000 entries");
            logger.info("WebServer", "HTTP server listening on port 8080");
            
            // Simulate some user activity
            logger.info("UserService", "User 'john' logged in");
            logger.info("OrderService", "Order #12345 created");
            logger.warn("PaymentService", "Payment gateway slow response (2.5s)");
            
            // Simulate errors
            logger.error("DatabasePool", "Connection pool exhausted, waiting...");
            logger.info("DatabasePool", "Connection acquired after 500ms");
            
            // Log with metadata
            logger.logWithMetadata(LogLevel.INFO, "MetricsService", "Metrics snapshot", 
                Map.of(
                    "cpu", "45%",
                    "memory", "2.1GB",
                    "connections", "150"
                ));
            
            logger.error("ExternalAPI", "Failed to call payment API: timeout");
            logger.warn("Security", "Multiple failed login attempts for user 'admin'");
            
            // Simulate critical error
            logger.fatal("Application", "OutOfMemoryError in worker thread!");
            
            // Display logs
            System.out.println("=== Last 10 Log Entries ===\n");
            for (LogEvent event : logger.getLast(10)) {
                System.out.println(event);
            }
            
            // Display only errors
            System.out.println("\n=== Error Events ===\n");
            for (LogEvent event : logger.getByLevel(LogLevel.ERROR, 10)) {
                System.out.println(event);
            }
            
            // Statistics
            System.out.println("\n=== Statistics ===");
            System.out.println("Total log entries: " + logger.size());
            System.out.println("Errors: " + logger.getByLevel(LogLevel.ERROR, 1000).size());
            System.out.println("Warnings: " + logger.getByLevel(LogLevel.WARN, 1000).size());
            
        } finally {
            logger.close();
        }
    }
}
