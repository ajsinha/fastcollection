#!/usr/bin/env python3
"""
FastCollection v1.0.0 - Task Queue Example (Python)

Implements a persistent task queue with priorities and dead letter queue.

Usage: python task_queue_example.py

Copyright © 2025-2030, Ashutosh Sinha (ajsinha@gmail.com)
Patent Pending
"""

from fastcollection import FastQueue
import json
import time
import random
from dataclasses import dataclass, asdict
from typing import Optional


@dataclass
class Task:
    id: str
    task_type: str
    payload: dict
    priority: int = 5  # 0 = highest priority
    created_at: float = None
    retry_count: int = 0
    max_retries: int = 3
    
    def __post_init__(self):
        if self.created_at is None:
            self.created_at = time.time()
    
    def to_json(self) -> bytes:
        return json.dumps(asdict(self)).encode()
    
    @classmethod
    def from_json(cls, data: bytes) -> 'Task':
        return cls(**json.loads(data.decode()))
    
    def should_retry(self) -> bool:
        return self.retry_count < self.max_retries


class TaskQueue:
    def __init__(self, base_path: str, task_ttl: int = 3600):
        self.queue = FastQueue(f"{base_path}/tasks.fc")
        self.dlq = FastQueue(f"{base_path}/dlq.fc")
        self.task_ttl = task_ttl
    
    def submit(self, task: Task):
        """Submit a task to the queue"""
        if task.priority == 0:
            # High priority - add to front
            self.queue.offer_first(task.to_json(), ttl=self.task_ttl)
        else:
            self.queue.offer(task.to_json(), ttl=self.task_ttl)
        print(f"Submitted: {task.id} ({task.task_type}) priority={task.priority}")
    
    def poll(self) -> Optional[Task]:
        """Get next task from queue"""
        data = self.queue.poll()
        if data is None:
            return None
        return Task.from_json(data)
    
    def peek(self) -> Optional[Task]:
        """Peek at next task without removing"""
        data = self.queue.peek()
        if data is None:
            return None
        return Task.from_json(data)
    
    def requeue(self, task: Task):
        """Requeue a failed task or move to DLQ"""
        task.retry_count += 1
        
        if task.should_retry():
            print(f"  Requeuing: {task.id} (attempt {task.retry_count}/{task.max_retries})")
            self.queue.offer(task.to_json(), ttl=self.task_ttl)
        else:
            print(f"  Moving to DLQ: {task.id} (max retries exceeded)")
            self.dlq.offer(task.to_json(), ttl=-1)  # Keep forever in DLQ
    
    def size(self) -> int:
        return self.queue.size()
    
    def dlq_size(self) -> int:
        return self.dlq.size()
    
    def close(self):
        self.queue.close()
        self.dlq.close()
    
    def __enter__(self):
        return self
    
    def __exit__(self, *args):
        self.close()


def process_task(task: Task) -> bool:
    """Simulate task processing with random failures"""
    print(f"Processing: {task.id} ({task.task_type})")
    time.sleep(0.1)  # Simulate work
    
    # 30% chance of failure
    if random.random() < 0.3:
        print(f"  FAILED!")
        return False
    
    print(f"  SUCCESS!")
    return True


def main():
    print("FastCollection v1.0.0 - Task Queue Example (Python)")
    print("=" * 55)
    print()
    
    with TaskQueue("/tmp/taskqueue_py", task_ttl=3600) as tq:
        # Submit various tasks
        print("Submitting tasks...\n")
        
        tasks = [
            Task("t1", "EMAIL", {"to": "user@example.com", "subject": "Welcome"}),
            Task("t2", "REPORT", {"type": "daily", "format": "pdf"}, priority=3),
            Task("t3", "ALERT", {"message": "Critical system alert!"}, priority=0),
            Task("t4", "BACKUP", {"database": "users", "destination": "s3"}, priority=5),
            Task("t5", "NOTIFY", {"user_id": "123", "message": "New message"}, priority=1),
        ]
        
        for task in tasks:
            tq.submit(task)
        
        print(f"\nQueue size: {tq.size()}")
        
        next_task = tq.peek()
        if next_task:
            print(f"Next task: {next_task.id} ({next_task.task_type})")
        
        # Process tasks
        print("\n--- Processing Tasks ---\n")
        
        while True:
            task = tq.poll()
            if task is None:
                break
            
            if not process_task(task):
                tq.requeue(task)
        
        print("\n--- Summary ---")
        print(f"Queue size: {tq.size()}")
        print(f"Dead letter queue size: {tq.dlq_size()}")


if __name__ == "__main__":
    main()
