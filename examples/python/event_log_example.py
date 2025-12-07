#!/usr/bin/env python3
"""
FastCollection v1.0.0 - Event Log Example (Python)

Implements a persistent event log with retention and filtering.

Usage: python event_log_example.py

Copyright © 2025-2030, Ashutosh Sinha (ajsinha@gmail.com)
Patent Pending
"""

from fastcollection import FastList
import json
import time
from datetime import datetime
from dataclasses import dataclass, field, asdict
from typing import List, Optional, Dict, Any
from enum import Enum
import threading


class LogLevel(Enum):
    DEBUG = 0
    INFO = 1
    WARN = 2
    ERROR = 3
    FATAL = 4


@dataclass
class LogEvent:
    level: str
    message: str
    source: str
    timestamp: float = field(default_factory=time.time)
    thread_name: str = field(default_factory=lambda: threading.current_thread().name)
    metadata: Dict[str, Any] = field(default_factory=dict)
    
    def to_json(self) -> bytes:
        return json.dumps(asdict(self)).encode()
    
    @classmethod
    def from_json(cls, data: bytes) -> 'LogEvent':
        d = json.loads(data.decode())
        return cls(**d)
    
    def format(self) -> str:
        ts = datetime.fromtimestamp(self.timestamp).strftime("%Y-%m-%d %H:%M:%S.%f")[:-3]
        base = f"[{ts}] {self.level:5} [{self.thread_name}] {self.source}: {self.message}"
        
        if self.metadata:
            base += f" | {self.metadata}"
        
        return base


class EventLogger:
    """
    Persistent event logger with TTL-based retention.
    """
    
    def __init__(self, name: str, path: str, retention_days: int = 7):
        self.name = name
        self.logs = FastList(path)
        self.retention_seconds = retention_days * 24 * 60 * 60
    
    def _log(self, level: str, source: str, message: str, **metadata):
        event = LogEvent(level=level, source=source, message=message, metadata=metadata)
        self.logs.add(event.to_json(), ttl=self.retention_seconds)
    
    def debug(self, source: str, message: str, **metadata):
        self._log("DEBUG", source, message, **metadata)
    
    def info(self, source: str, message: str, **metadata):
        self._log("INFO", source, message, **metadata)
    
    def warn(self, source: str, message: str, **metadata):
        self._log("WARN", source, message, **metadata)
    
    def error(self, source: str, message: str, **metadata):
        self._log("ERROR", source, message, **metadata)
    
    def fatal(self, source: str, message: str, **metadata):
        self._log("FATAL", source, message, **metadata)
    
    def get_last(self, count: int = 10) -> List[LogEvent]:
        """Get last N log entries"""
        result = []
        size = self.logs.size()
        start = max(0, size - count)
        
        for i in range(start, size):
            data = self.logs.get(i)
            if data:
                result.append(LogEvent.from_json(data))
        
        return result
    
    def get_by_level(self, level: str, max_count: int = 100) -> List[LogEvent]:
        """Get logs of a specific level"""
        result = []
        size = self.logs.size()
        
        for i in range(size - 1, -1, -1):
            if len(result) >= max_count:
                break
            
            data = self.logs.get(i)
            if data:
                event = LogEvent.from_json(data)
                if event.level == level:
                    result.append(event)
        
        result.reverse()
        return result
    
    def get_by_source(self, source: str, max_count: int = 100) -> List[LogEvent]:
        """Get logs from a specific source"""
        result = []
        size = self.logs.size()
        
        for i in range(size - 1, -1, -1):
            if len(result) >= max_count:
                break
            
            data = self.logs.get(i)
            if data:
                event = LogEvent.from_json(data)
                if event.source == source:
                    result.append(event)
        
        result.reverse()
        return result
    
    def search(self, keyword: str, max_count: int = 100) -> List[LogEvent]:
        """Search logs for keyword"""
        result = []
        size = self.logs.size()
        keyword_lower = keyword.lower()
        
        for i in range(size - 1, -1, -1):
            if len(result) >= max_count:
                break
            
            data = self.logs.get(i)
            if data:
                event = LogEvent.from_json(data)
                if keyword_lower in event.message.lower() or keyword_lower in event.source.lower():
                    result.append(event)
        
        result.reverse()
        return result
    
    def cleanup(self) -> int:
        """Remove expired log entries"""
        return self.logs.remove_expired()
    
    def size(self) -> int:
        return self.logs.size()
    
    def close(self):
        self.logs.close()
    
    def __enter__(self):
        return self
    
    def __exit__(self, *args):
        self.close()


def main():
    print("FastCollection v1.0.0 - Event Log Example (Python)")
    print("=" * 52)
    print()
    
    # Create logger with 7-day retention
    with EventLogger("AppLogger", "/tmp/eventlog_py.fc", retention_days=7) as logger:
        # Simulate application events
        print("Logging events...\n")
        
        logger.info("Application", "Application started", version="1.0.0", env="production")
        logger.info("Database", "Connected to PostgreSQL", host="localhost", port=5432)
        logger.debug("Cache", "Initialized cache", max_entries=1000)
        logger.info("WebServer", "HTTP server listening", port=8080, ssl=True)
        
        # User activity
        logger.info("UserService", "User logged in", user_id="john_doe", ip="192.168.1.100")
        logger.info("OrderService", "Order created", order_id=12345, amount=99.99)
        logger.warn("PaymentService", "Slow payment gateway", response_time_ms=2500)
        
        # Errors
        logger.error("DatabasePool", "Connection pool exhausted", active=100, max=100)
        logger.info("DatabasePool", "Connection acquired", wait_ms=500)
        
        # Metrics
        logger.info("MetricsService", "Metrics snapshot", cpu="45%", memory="2.1GB", connections=150)
        
        # More errors
        logger.error("ExternalAPI", "Payment API timeout", endpoint="/v1/charge", timeout_ms=5000)
        logger.warn("Security", "Failed login attempts", user="admin", attempts=5, blocked=True)
        
        # Critical
        logger.fatal("Application", "OutOfMemoryError in worker thread!", heap_used="7.8GB", heap_max="8GB")
        
        # Display logs
        print("=== Last 10 Log Entries ===\n")
        for event in logger.get_last(10):
            print(event.format())
        
        # Filter by level
        print("\n=== ERROR Logs ===\n")
        for event in logger.get_by_level("ERROR", 10):
            print(event.format())
        
        # Filter by source
        print("\n=== Database Logs ===\n")
        for event in logger.get_by_source("DatabasePool", 10):
            print(event.format())
        
        # Search
        print("\n=== Search: 'timeout' ===\n")
        for event in logger.search("timeout", 10):
            print(event.format())
        
        # Statistics
        print("\n=== Statistics ===")
        print(f"Total entries: {logger.size()}")
        print(f"Errors: {len(logger.get_by_level('ERROR', 1000))}")
        print(f"Warnings: {len(logger.get_by_level('WARN', 1000))}")


if __name__ == "__main__":
    main()
