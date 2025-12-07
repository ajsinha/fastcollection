/**
 * Copyright © 2025-2030, All Rights Reserved
 * Ashutosh Sinha | Email: ajsinha@gmail.com
 * Patent Pending
 */
package com.abhikarta.fastcollection;

import org.junit.jupiter.api.*;
import java.io.File;
import static org.junit.jupiter.api.Assertions.*;

@TestMethodOrder(MethodOrderer.OrderAnnotation.class)
public class FastCollectionMapTest {
    
    private static final String TEST_FILE = "/tmp/test_fc_map.fc";
    private FastCollectionMap<String, String> map;
    
    @BeforeEach
    void setUp() {
        new File(TEST_FILE).delete();
        map = new FastCollectionMap<>(TEST_FILE, String.class, String.class, 16 * 1024 * 1024, true);
    }
    
    @AfterEach
    void tearDown() {
        if (map != null) {
            map.close();
        }
    }
    
    @Test
    @Order(1)
    void testBasicOperations() {
        assertTrue(map.isEmpty());
        assertEquals(0, map.size());
        
        map.put("key1", "value1");
        map.put("key2", "value2");
        
        assertFalse(map.isEmpty());
        assertEquals(2, map.size());
        
        assertEquals("value1", map.get("key1"));
        assertEquals("value2", map.get("key2"));
        
        map.clear();
        assertTrue(map.isEmpty());
    }
    
    @Test
    @Order(2)
    void testTTL() throws InterruptedException {
        // Add with 1 second TTL
        map.put("temp", "value", 1);
        
        assertEquals(1, map.size());
        assertEquals("value", map.get("temp"));
        
        // Wait for expiration
        Thread.sleep(2000);
        
        // Should be expired
        assertEquals(0, map.size());
        assertNull(map.get("temp"));
    }
    
    @Test
    @Order(3)
    void testPutIfAbsent() {
        assertTrue(map.putIfAbsent("key", "first"));
        assertFalse(map.putIfAbsent("key", "second"));
        
        assertEquals("first", map.get("key"));
    }
    
    @Test
    @Order(4)
    void testRemove() {
        map.put("key", "value");
        assertTrue(map.containsKey("key"));
        
        assertEquals("value", map.remove("key"));
        assertFalse(map.containsKey("key"));
    }
    
    @Test
    @Order(5)
    void testContainsKey() {
        map.put("exists", "yes");
        
        assertTrue(map.containsKey("exists"));
        assertFalse(map.containsKey("not_exists"));
    }
    
    @Test
    @Order(6)
    void testReplace() {
        map.put("key", "old");
        
        assertTrue(map.replace("key", "new"));
        assertEquals("new", map.get("key"));
        
        assertFalse(map.replace("nonexistent", "value"));
    }
    
    @Test
    @Order(7)
    void testTTLUpdate() {
        map.put("key", "value", 10);
        
        assertTrue(map.setTTL("key", 60));
        long ttl = map.getTTL("key");
        assertTrue(ttl > 50 && ttl <= 60);
    }
}
