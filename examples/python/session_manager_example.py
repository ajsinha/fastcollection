#!/usr/bin/env python3
"""
FastCollection v1.0.0 - Session Manager Example (Python)

Implements a persistent session manager with auto-expiry.

Usage: python session_manager_example.py

Copyright © 2025-2030, Ashutosh Sinha (ajsinha@gmail.com)
Patent Pending
"""

from fastcollection import FastMap
import json
import time
import uuid
from dataclasses import dataclass, field, asdict
from typing import Optional, Any, Dict


@dataclass
class Session:
    session_id: str
    user_id: str
    username: str
    created_at: float = field(default_factory=time.time)
    last_accessed_at: float = field(default_factory=time.time)
    attributes: Dict[str, Any] = field(default_factory=dict)
    
    def touch(self):
        """Update last accessed time"""
        self.last_accessed_at = time.time()
    
    def set_attribute(self, key: str, value: Any):
        self.attributes[key] = value
        self.touch()
    
    def get_attribute(self, key: str, default: Any = None) -> Any:
        self.touch()
        return self.attributes.get(key, default)
    
    def to_json(self) -> bytes:
        return json.dumps(asdict(self)).encode()
    
    @classmethod
    def from_json(cls, data: bytes) -> 'Session':
        return cls(**json.loads(data.decode()))
    
    def __str__(self):
        return f"Session({self.session_id[:8]}..., user={self.username}, attrs={len(self.attributes)})"


class SessionManager:
    def __init__(self, path: str, session_timeout_minutes: int = 30):
        self.store = FastMap(path)
        self.session_timeout = session_timeout_minutes * 60
    
    def create_session(self, user_id: str, username: str) -> Session:
        """Create a new session"""
        session_id = str(uuid.uuid4())
        session = Session(session_id, user_id, username)
        self.store.put(session_id.encode(), session.to_json(), ttl=self.session_timeout)
        print(f"Created session: {session}")
        return session
    
    def get_session(self, session_id: str) -> Optional[Session]:
        """Get a session and refresh its TTL"""
        data = self.store.get(session_id.encode())
        if data is None:
            return None
        
        session = Session.from_json(data)
        session.touch()
        
        # Refresh TTL
        self.store.put(session_id.encode(), session.to_json(), ttl=self.session_timeout)
        return session
    
    def update_session(self, session: Session):
        """Update session data"""
        self.store.put(session.session_id.encode(), session.to_json(), ttl=self.session_timeout)
    
    def invalidate_session(self, session_id: str):
        """Invalidate/delete a session"""
        self.store.remove(session_id.encode())
        print(f"Invalidated session: {session_id[:8]}...")
    
    def get_session_ttl(self, session_id: str) -> int:
        """Get remaining TTL for a session"""
        return self.store.get_ttl(session_id.encode())
    
    def active_session_count(self) -> int:
        """Get count of active sessions"""
        return self.store.size()
    
    def cleanup(self) -> int:
        """Remove expired sessions"""
        return self.store.remove_expired()
    
    def close(self):
        self.store.close()
    
    def __enter__(self):
        return self
    
    def __exit__(self, *args):
        self.close()


def main():
    print("FastCollection v1.0.0 - Session Manager Example (Python)")
    print("=" * 58)
    print()
    
    # Create session manager (sessions expire in 1 minute for demo)
    with SessionManager("/tmp/sessions_py.fc", session_timeout_minutes=1) as manager:
        # Create sessions for users
        print("Creating sessions...\n")
        
        session1 = manager.create_session("user_001", "john_doe")
        session2 = manager.create_session("user_002", "jane_smith")
        session3 = manager.create_session("user_003", "bob_wilson")
        
        # Store session attributes
        print("\nStoring session attributes...")
        
        session1.set_attribute("role", "admin")
        session1.set_attribute("theme", "dark")
        session1.set_attribute("cart", ["item1", "item2", "item3"])
        session1.set_attribute("preferences", {"notifications": True, "language": "en"})
        manager.update_session(session1)
        
        session2.set_attribute("role", "user")
        session2.set_attribute("last_page", "/dashboard")
        manager.update_session(session2)
        
        # Display active sessions
        print(f"\nActive sessions: {manager.active_session_count()}")
        
        # Retrieve and use session
        print("\nRetrieving session for john_doe...")
        retrieved = manager.get_session(session1.session_id)
        
        if retrieved:
            print(f"  User: {retrieved.username}")
            print(f"  Role: {retrieved.get_attribute('role')}")
            print(f"  Theme: {retrieved.get_attribute('theme')}")
            print(f"  Cart: {retrieved.get_attribute('cart')}")
            print(f"  Preferences: {retrieved.get_attribute('preferences')}")
            print(f"  TTL: {manager.get_session_ttl(session1.session_id)} seconds")
        
        # Invalidate one session
        print("\nInvalidating Bob's session...")
        manager.invalidate_session(session3.session_id)
        print(f"Active sessions: {manager.active_session_count()}")
        
        # Wait for sessions to expire (shortened for demo)
        print("\nWaiting 70 seconds for sessions to expire...")
        time.sleep(70)
        
        # Check what's left
        print("\nAfter timeout:")
        print(f"  Active sessions: {manager.active_session_count()}")
        
        # Cleanup
        removed = manager.cleanup()
        print(f"  Cleaned up {removed} expired sessions")
        
        print("\nExample completed successfully!")


if __name__ == "__main__":
    main()
