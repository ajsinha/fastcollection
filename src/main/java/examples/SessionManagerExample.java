/**
 * FastCollection v1.0.0 - Session Manager Example
 * 
 * This example demonstrates implementing a session manager with auto-expiry.
 * 
 * Copyright © 2025-2030, Ashutosh Sinha (ajsinha@gmail.com)
 * Patent Pending
 */
package examples;

import com.kuber.fastcollection.*;
import java.io.Serializable;
import java.util.*;

public class SessionManagerExample {
    
    /**
     * User session data
     */
    public static class Session implements Serializable {
        private static final long serialVersionUID = 1L;
        
        public final String sessionId;
        public final String userId;
        public final String username;
        public final long createdAt;
        public long lastAccessedAt;
        private final Map<String, Serializable> attributes;
        
        public Session(String sessionId, String userId, String username) {
            this.sessionId = sessionId;
            this.userId = userId;
            this.username = username;
            this.createdAt = System.currentTimeMillis();
            this.lastAccessedAt = this.createdAt;
            this.attributes = new HashMap<>();
        }
        
        public void setAttribute(String key, Serializable value) {
            attributes.put(key, value);
            lastAccessedAt = System.currentTimeMillis();
        }
        
        public Serializable getAttribute(String key) {
            lastAccessedAt = System.currentTimeMillis();
            return attributes.get(key);
        }
        
        public Set<String> getAttributeNames() {
            return attributes.keySet();
        }
        
        @Override
        public String toString() {
            return String.format("Session{id=%s, user=%s, attrs=%d}", 
                sessionId.substring(0, 8), username, attributes.size());
        }
    }
    
    /**
     * Session Manager with persistent storage
     */
    public static class SessionManager {
        private final FastCollectionMap<String, Session> sessions;
        private final int sessionTimeout;  // seconds
        
        public SessionManager(String storagePath, int sessionTimeoutMinutes) {
            this.sessions = new FastCollectionMap<>(storagePath, 64 * 1024 * 1024, true);
            this.sessionTimeout = sessionTimeoutMinutes * 60;
        }
        
        public Session createSession(String userId, String username) {
            String sessionId = UUID.randomUUID().toString();
            Session session = new Session(sessionId, userId, username);
            sessions.put(sessionId, session, sessionTimeout);
            System.out.println("Created session: " + session);
            return session;
        }
        
        public Session getSession(String sessionId) {
            Session session = sessions.get(sessionId);
            if (session != null) {
                // Touch session to refresh TTL
                session.lastAccessedAt = System.currentTimeMillis();
                sessions.put(sessionId, session, sessionTimeout);
            }
            return session;
        }
        
        public void updateSession(Session session) {
            sessions.put(session.sessionId, session, sessionTimeout);
        }
        
        public void invalidateSession(String sessionId) {
            Session session = sessions.get(sessionId);
            if (session != null) {
                System.out.println("Invalidated session: " + session);
                sessions.remove(sessionId);
            }
        }
        
        public int getActiveSessionCount() {
            return sessions.size();
        }
        
        public long getSessionTTL(String sessionId) {
            return sessions.getTTL(sessionId);
        }
        
        public void cleanup() {
            int removed = sessions.removeExpired();
            if (removed > 0) {
                System.out.println("Cleaned up " + removed + " expired sessions");
            }
        }
        
        public void close() {
            sessions.close();
        }
    }
    
    public static void main(String[] args) throws InterruptedException {
        System.out.println("FastCollection v1.0.0 - Session Manager Example");
        System.out.println("===============================================\n");
        
        // Create session manager (sessions expire in 1 minute for demo)
        SessionManager manager = new SessionManager("/tmp/sessions_example.fc", 1);
        
        try {
            // Create sessions for users
            System.out.println("Creating sessions...\n");
            Session session1 = manager.createSession("user_001", "John Doe");
            Session session2 = manager.createSession("user_002", "Jane Smith");
            Session session3 = manager.createSession("user_003", "Bob Wilson");
            
            // Store session attributes
            System.out.println("\nStoring session attributes...");
            session1.setAttribute("role", "admin");
            session1.setAttribute("theme", "dark");
            session1.setAttribute("cart", new ArrayList<>(Arrays.asList("item1", "item2")));
            manager.updateSession(session1);
            
            session2.setAttribute("role", "user");
            session2.setAttribute("preferences", "compact_view");
            manager.updateSession(session2);
            
            // Display active sessions
            System.out.println("\nActive sessions: " + manager.getActiveSessionCount());
            
            // Retrieve and use session
            System.out.println("\nRetrieving session for user John Doe...");
            Session retrieved = manager.getSession(session1.sessionId);
            if (retrieved != null) {
                System.out.println("  User: " + retrieved.username);
                System.out.println("  Role: " + retrieved.getAttribute("role"));
                System.out.println("  Theme: " + retrieved.getAttribute("theme"));
                System.out.println("  Cart: " + retrieved.getAttribute("cart"));
                System.out.println("  TTL: " + manager.getSessionTTL(session1.sessionId) + " seconds");
            }
            
            // Invalidate one session
            System.out.println("\nInvalidating Bob's session...");
            manager.invalidateSession(session3.sessionId);
            System.out.println("Active sessions: " + manager.getActiveSessionCount());
            
            // Wait for sessions to expire
            System.out.println("\nWaiting 70 seconds for sessions to expire...");
            Thread.sleep(70000);
            
            // Check what's left
            System.out.println("\nAfter timeout:");
            System.out.println("  Active sessions: " + manager.getActiveSessionCount());
            
            // Cleanup
            manager.cleanup();
            
            System.out.println("\nExample completed successfully!");
            
        } finally {
            manager.close();
        }
    }
}
