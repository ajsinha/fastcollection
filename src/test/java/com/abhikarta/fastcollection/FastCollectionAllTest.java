/**
 * Copyright © 2025-2030, All Rights Reserved
 * Ashutosh Sinha | Email: ajsinha@gmail.com
 * Patent Pending
 */
package com.abhikarta.fastcollection;

import org.junit.jupiter.api.*;
import java.io.File;
import static org.junit.jupiter.api.Assertions.*;

public class FastCollectionAllTest {

    // ========== FastSet Tests ==========
    @Nested
    @TestMethodOrder(MethodOrderer.OrderAnnotation.class)
    class SetTests {
        private static final String TEST_FILE = "/tmp/test_fc_set.fc";
        private FastCollectionSet<String> set;
        
        @BeforeEach
        void setUp() {
            new File(TEST_FILE).delete();
            set = new FastCollectionSet<>(TEST_FILE, String.class, 16 * 1024 * 1024, true);
        }
        
        @AfterEach
        void tearDown() {
            if (set != null) set.close();
        }
        
        @Test
        @Order(1)
        void testBasicOperations() {
            assertTrue(set.isEmpty());
            
            assertTrue(set.add("hello"));
            assertFalse(set.add("hello")); // Duplicate
            assertTrue(set.add("world"));
            
            assertEquals(2, set.size());
            assertTrue(set.contains("hello"));
            assertTrue(set.contains("world"));
            assertFalse(set.contains("foo"));
        }
        
        @Test
        @Order(2)
        void testTTL() throws InterruptedException {
            set.add("temp", 1);
            assertEquals(1, set.size());
            
            Thread.sleep(2000);
            assertEquals(0, set.size());
        }
        
        @Test
        @Order(3)
        void testRemove() {
            set.add("item");
            assertTrue(set.remove("item"));
            assertFalse(set.remove("item"));
            assertFalse(set.contains("item"));
        }
    }

    // ========== FastQueue Tests ==========
    @Nested
    @TestMethodOrder(MethodOrderer.OrderAnnotation.class)
    class QueueTests {
        private static final String TEST_FILE = "/tmp/test_fc_queue.fc";
        private FastCollectionQueue<String> queue;
        
        @BeforeEach
        void setUp() {
            new File(TEST_FILE).delete();
            queue = new FastCollectionQueue<>(TEST_FILE, String.class, 16 * 1024 * 1024, true);
        }
        
        @AfterEach
        void tearDown() {
            if (queue != null) queue.close();
        }
        
        @Test
        @Order(1)
        void testFIFO() {
            queue.offer("first");
            queue.offer("second");
            queue.offer("third");
            
            assertEquals("first", queue.poll());
            assertEquals("second", queue.poll());
            assertEquals("third", queue.poll());
            assertNull(queue.poll());
        }
        
        @Test
        @Order(2)
        void testPeek() {
            queue.offer("data");
            assertEquals("data", queue.peek());
            assertEquals("data", queue.peek()); // Still there
            assertEquals(1, queue.size());
        }
        
        @Test
        @Order(3)
        void testTTL() throws InterruptedException {
            queue.offer("temp", 1);
            assertEquals(1, queue.size());
            
            Thread.sleep(2000);
            assertNull(queue.poll()); // Expired
        }
        
        @Test
        @Order(4)
        void testDequeOperations() {
            queue.offerFirst("middle");
            queue.offerFirst("first");
            queue.offer("last");
            
            assertEquals("first", queue.poll());
            assertEquals("last", queue.pollLast());
            assertEquals("middle", queue.poll());
        }
    }

    // ========== FastStack Tests ==========
    @Nested
    @TestMethodOrder(MethodOrderer.OrderAnnotation.class)
    class StackTests {
        private static final String TEST_FILE = "/tmp/test_fc_stack.fc";
        private FastCollectionStack<String> stack;
        
        @BeforeEach
        void setUp() {
            new File(TEST_FILE).delete();
            stack = new FastCollectionStack<>(TEST_FILE, String.class, 16 * 1024 * 1024, true);
        }
        
        @AfterEach
        void tearDown() {
            if (stack != null) stack.close();
        }
        
        @Test
        @Order(1)
        void testLIFO() {
            stack.push("first");
            stack.push("second");
            stack.push("third");
            
            assertEquals("third", stack.pop());
            assertEquals("second", stack.pop());
            assertEquals("first", stack.pop());
            assertNull(stack.pop());
        }
        
        @Test
        @Order(2)
        void testPeek() {
            stack.push("data");
            assertEquals("data", stack.peek());
            assertEquals("data", stack.peek()); // Still there
            assertEquals(1, stack.size());
        }
        
        @Test
        @Order(3)
        void testTTL() throws InterruptedException {
            stack.push("temp", 1);
            assertEquals(1, stack.size());
            
            Thread.sleep(2000);
            assertNull(stack.pop()); // Expired
        }
        
        @Test
        @Order(4)
        void testSearch() {
            stack.push("a");
            stack.push("b");
            stack.push("c");
            
            assertEquals(1, stack.search("c")); // Top
            assertEquals(2, stack.search("b"));
            assertEquals(3, stack.search("a")); // Bottom
            assertEquals(-1, stack.search("x"));
        }
    }
}
