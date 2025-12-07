/**
 * Copyright © 2025-2030, All Rights Reserved
 * Ashutosh Sinha | Email: ajsinha@gmail.com
 * Patent Pending
 */
package com.abhikarta.fastcollection;

import org.junit.jupiter.api.*;
import java.io.File;
import java.nio.charset.StandardCharsets;
import static org.junit.jupiter.api.Assertions.*;

@TestMethodOrder(MethodOrderer.OrderAnnotation.class)
public class FastCollectionListTest {
    
    private static final String TEST_FILE = "/tmp/test_fc_list.fc";
    private FastCollectionList<String> list;
    
    @BeforeEach
    void setUp() {
        new File(TEST_FILE).delete();
        list = new FastCollectionList<>(TEST_FILE, String.class, 16 * 1024 * 1024, true);
    }
    
    @AfterEach
    void tearDown() {
        if (list != null) {
            list.close();
        }
    }
    
    @Test
    @Order(1)
    void testBasicOperations() {
        assertTrue(list.isEmpty());
        assertEquals(0, list.size());
        
        list.add("hello");
        list.add("world");
        
        assertFalse(list.isEmpty());
        assertEquals(2, list.size());
        
        assertEquals("hello", list.get(0));
        assertEquals("world", list.get(1));
        
        list.clear();
        assertTrue(list.isEmpty());
    }
    
    @Test
    @Order(2)
    void testTTLInfinite() {
        // TTL = -1 means never expires (default)
        list.add("permanent");
        
        long ttl = list.getTTL(0);
        assertEquals(-1, ttl, "TTL should be -1 (infinite)");
        
        assertEquals("permanent", list.get(0));
    }
    
    @Test
    @Order(3)
    void testTTLExpiration() throws InterruptedException {
        // Add with 1 second TTL
        list.add("temporary", 1);
        
        assertEquals(1, list.size());
        assertNotNull(list.get(0));
        
        // Wait for expiration
        Thread.sleep(2000);
        
        // Should be expired
        assertEquals(0, list.size());
        
        // Remove expired
        int removed = list.removeExpired();
        assertEquals(1, removed);
    }
    
    @Test
    @Order(4)
    void testTTLUpdate() {
        list.add("test", 10);
        
        // Update TTL
        assertTrue(list.setTTL(0, 60));
        
        long ttl = list.getTTL(0);
        assertTrue(ttl > 50 && ttl <= 60);
        
        // Update to infinite
        assertTrue(list.setTTL(0, -1));
        assertEquals(-1, list.getTTL(0));
    }
    
    @Test
    @Order(5)
    void testAddFirst() {
        list.add("second");
        list.addFirst("first");
        
        assertEquals("first", list.get(0));
        assertEquals("second", list.get(1));
    }
    
    @Test
    @Order(6)
    void testRemove() {
        list.add("a");
        list.add("b");
        list.add("c");
        
        assertEquals("b", list.remove(1));
        assertEquals(2, list.size());
        assertEquals("c", list.get(1));
    }
    
    @Test
    @Order(7)
    void testContains() {
        list.add("hello");
        list.add("world");
        
        assertTrue(list.contains("hello"));
        assertTrue(list.contains("world"));
        assertFalse(list.contains("foo"));
    }
    
    @Test
    @Order(8)
    void testIndexOf() {
        list.add("a");
        list.add("b");
        list.add("c");
        
        assertEquals(0, list.indexOf("a"));
        assertEquals(1, list.indexOf("b"));
        assertEquals(2, list.indexOf("c"));
        assertEquals(-1, list.indexOf("x"));
    }
    
    @Test
    @Order(9)
    void testIterator() {
        list.add("one");
        list.add("two");
        list.add("three");
        
        StringBuilder sb = new StringBuilder();
        for (String s : list) {
            sb.append(s).append(",");
        }
        assertEquals("one,two,three,", sb.toString());
    }
    
    @Test
    @Order(10)
    void testMixedTTL() throws InterruptedException {
        list.add("permanent", -1);  // Never expires
        list.add("short", 1);       // 1 second
        list.add("long", 60);       // 60 seconds
        
        assertEquals(3, list.size());
        
        Thread.sleep(2000);
        
        // Short should be expired
        assertEquals(2, list.size());
    }
}
