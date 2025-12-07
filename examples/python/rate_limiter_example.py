#!/usr/bin/env python3
"""
FastCollection v1.0.0 - Rate Limiter Example (Python)

Implements a sliding window rate limiter.

Usage: python rate_limiter_example.py

Copyright © 2025-2030, Ashutosh Sinha (ajsinha@gmail.com)
Patent Pending
"""

from fastcollection import FastMap
import json
import time
from dataclasses import dataclass, asdict


@dataclass
class RateLimitEntry:
    count: int
    window_start: float
    
    def to_json(self) -> bytes:
        return json.dumps(asdict(self)).encode()
    
    @classmethod
    def from_json(cls, data: bytes) -> 'RateLimitEntry':
        return cls(**json.loads(data.decode()))


class RateLimiter:
    """
    Sliding window rate limiter with persistent storage.
    
    Example: 100 requests per 60 seconds
    """
    
    def __init__(self, path: str, max_requests: int, window_seconds: int):
        self.store = FastMap(path)
        self.max_requests = max_requests
        self.window_seconds = window_seconds
    
    def allow_request(self, client_id: str) -> bool:
        """Check if request is allowed and update counter"""
        key = client_id.encode()
        data = self.store.get(key)
        
        if data is None:
            # First request in window
            entry = RateLimitEntry(count=1, window_start=time.time())
            self.store.put(key, entry.to_json(), ttl=self.window_seconds)
            return True
        
        entry = RateLimitEntry.from_json(data)
        
        if entry.count >= self.max_requests:
            return False
        
        # Increment counter
        entry.count += 1
        self.store.put(key, entry.to_json(), ttl=self.window_seconds)
        return True
    
    def get_remaining(self, client_id: str) -> int:
        """Get remaining requests in current window"""
        data = self.store.get(client_id.encode())
        
        if data is None:
            return self.max_requests
        
        entry = RateLimitEntry.from_json(data)
        return max(0, self.max_requests - entry.count)
    
    def get_reset_time(self, client_id: str) -> int:
        """Get seconds until rate limit resets"""
        return self.store.get_ttl(client_id.encode())
    
    def close(self):
        self.store.close()
    
    def __enter__(self):
        return self
    
    def __exit__(self, *args):
        self.close()


class MultiTierRateLimiter:
    """
    Multi-tier rate limiter with different limits.
    
    Example:
    - 10 requests per second (burst protection)
    - 100 requests per minute (short-term limit)
    - 1000 requests per hour (long-term limit)
    """
    
    def __init__(self, base_path: str):
        self.limiters = {
            'second': RateLimiter(f"{base_path}/sec.fc", 10, 1),
            'minute': RateLimiter(f"{base_path}/min.fc", 100, 60),
            'hour': RateLimiter(f"{base_path}/hour.fc", 1000, 3600),
        }
    
    def allow_request(self, client_id: str) -> tuple:
        """
        Check all tiers. Returns (allowed, limiting_tier or None)
        """
        for tier_name, limiter in self.limiters.items():
            if not limiter.allow_request(client_id):
                return False, tier_name
        return True, None
    
    def get_status(self, client_id: str) -> dict:
        """Get rate limit status for all tiers"""
        return {
            tier: {
                'remaining': limiter.get_remaining(client_id),
                'limit': limiter.max_requests,
                'reset_in': limiter.get_reset_time(client_id)
            }
            for tier, limiter in self.limiters.items()
        }
    
    def close(self):
        for limiter in self.limiters.values():
            limiter.close()
    
    def __enter__(self):
        return self
    
    def __exit__(self, *args):
        self.close()


def demo_simple_rate_limiter():
    print("=== Simple Rate Limiter ===\n")
    
    # 10 requests per 60 seconds
    with RateLimiter("/tmp/ratelimit_simple.fc", max_requests=10, window_seconds=60) as limiter:
        client_id = "api_client_001"
        
        print(f"Rate limit: 10 requests per 60 seconds\n")
        
        for i in range(1, 16):
            allowed = limiter.allow_request(client_id)
            remaining = limiter.get_remaining(client_id)
            
            status = "✓ ALLOWED" if allowed else "✗ DENIED "
            print(f"Request {i:2d}: {status} (remaining: {remaining})")
        
        print(f"\nReset in: {limiter.get_reset_time(client_id)} seconds")


def demo_multi_tier_rate_limiter():
    print("\n=== Multi-Tier Rate Limiter ===\n")
    
    with MultiTierRateLimiter("/tmp/ratelimit_multi") as limiter:
        client_id = "api_client_002"
        
        print("Tiers:")
        print("  - Second: 10 req/sec")
        print("  - Minute: 100 req/min")
        print("  - Hour: 1000 req/hour")
        print()
        
        # Simulate burst of requests
        for i in range(1, 16):
            allowed, tier = limiter.allow_request(client_id)
            
            if allowed:
                print(f"Request {i:2d}: ✓ ALLOWED")
            else:
                print(f"Request {i:2d}: ✗ DENIED (limit: {tier})")
        
        # Show status
        print("\nCurrent status:")
        status = limiter.get_status(client_id)
        
        for tier, info in status.items():
            print(f"  {tier}: {info['remaining']}/{info['limit']} remaining, "
                  f"resets in {info['reset_in']}s")


def main():
    print("FastCollection v1.0.0 - Rate Limiter Example (Python)")
    print("=" * 55)
    print()
    
    demo_simple_rate_limiter()
    demo_multi_tier_rate_limiter()
    
    print("\nExample completed successfully!")


if __name__ == "__main__":
    main()
