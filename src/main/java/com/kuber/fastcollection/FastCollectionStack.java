/**
 * Copyright © 2025-2030, All Rights Reserved
 * Ashutosh Sinha | Email: ajsinha@gmail.com
 * Patent Pending
 */
package com.kuber.fastcollection;

import java.io.*;
import java.util.*;

/**
 * Ultra high-performance memory-mapped LIFO stack with TTL support.
 * <p>
 * Uses lock-free CAS operations for push/pop with ABA prevention.
 * Expired elements are automatically skipped during pop operations.
 * 
 * <h2>TTL Behavior</h2>
 * <pre>
 * stack.push(state);              // Never expires
 * stack.push(state, 300);         // Expires in 5 minutes
 * stack.pop();                    // Returns top non-expired element
 * </pre>
 * 
 * @param <T> element type (must be Serializable)
 */
public class FastCollectionStack<T extends Serializable> implements Closeable, Iterable<T> {
    
    /** Constant indicating infinite TTL (element never expires). */
    public static final int TTL_INFINITE = -1;
    
    /** Default initial size for the memory-mapped file (64 MB). */
    public static final long DEFAULT_INITIAL_SIZE = 64L * 1024 * 1024;
    
    private final long nativeHandle;
    private final String filePath;
    private volatile boolean closed = false;
    
    static { NativeLibraryLoader.load(); }
    
    private static native long nativeCreate(String filePath, long initialSize, boolean createNew);
    private static native void nativeDestroy(long handle);
    private native boolean nativePush(long handle, byte[] data, int ttlSeconds);
    private native byte[] nativePop(long handle);
    private native byte[] nativePeek(long handle);
    private native long nativePeekTTL(long handle);
    private native long nativeSearch(long handle, byte[] data);
    private native int nativeRemoveExpired(long handle);
    private native void nativeClear(long handle);
    private native int nativeSize(long handle);
    private native boolean nativeIsEmpty(long handle);
    private native void nativeFlush(long handle);
    
    /**
     * Creates a new FastCollectionStack.
     * @param filePath path to memory-mapped file
     */
    public FastCollectionStack(String filePath) { this(filePath, DEFAULT_INITIAL_SIZE, false); }
    
    /**
     * Creates a new FastCollectionStack with specified size.
     * @param filePath path to memory-mapped file
     * @param initialSize initial size in bytes
     * @param createNew if true, truncate existing file
     */
    public FastCollectionStack(String filePath, long initialSize, boolean createNew) {
        this.filePath = filePath;
        this.nativeHandle = nativeCreate(filePath, initialSize, createNew);
        if (this.nativeHandle == 0) throw new FastCollectionException("Failed to create stack: " + filePath);
    }
    
    /**
     * Push element with TTL.
     * @param element the element to push
     * @param ttlSeconds TTL in seconds (-1 for infinite, never expires)
     * @return true if successful
     */
    public boolean push(T element, int ttlSeconds) {
        checkClosed();
        return nativePush(nativeHandle, serialize(element), ttlSeconds);
    }
    
    /**
     * Push element without TTL (never expires).
     * @param element the element to push
     * @return true if successful
     */
    public boolean push(T element) {
        return push(element, TTL_INFINITE);
    }
    
    /**
     * Pop element from top.
     * @return the element, or null if stack is empty
     */
    @SuppressWarnings("unchecked")
    public T pop() {
        checkClosed();
        byte[] data = nativePop(nativeHandle);
        return data != null ? deserialize(data) : null;
    }
    
    /**
     * Peek at top element without removing.
     * @return the element, or null if empty
     */
    @SuppressWarnings("unchecked")
    public T peek() {
        checkClosed();
        byte[] data = nativePeek(nativeHandle);
        return data != null ? deserialize(data) : null;
    }
    
    /**
     * Pop element or throw if empty.
     * @return the element
     * @throws EmptyStackException if stack is empty
     */
    public T popOrThrow() {
        T element = pop();
        if (element == null) throw new EmptyStackException();
        return element;
    }
    
    /**
     * Get remaining TTL of top element.
     * @return remaining seconds, -1 if infinite, 0 if expired/empty
     */
    public long peekTTL() {
        checkClosed();
        return nativePeekTTL(nativeHandle);
    }
    
    /**
     * Search for element and return 1-based distance from top.
     * @param element the element to search for
     * @return 1-based distance (1 = top), or -1 if not found
     */
    @SuppressWarnings("unchecked")
    public int search(T element) {
        checkClosed();
        return (int) nativeSearch(nativeHandle, serialize(element));
    }
    
    /**
     * Check if stack contains element.
     * @param element the element to search for
     * @return true if found and not expired
     */
    public boolean contains(T element) {
        return search(element) > 0;
    }
    
    /**
     * Remove all expired elements.
     * @return number of elements removed
     */
    public int removeExpired() {
        checkClosed();
        return nativeRemoveExpired(nativeHandle);
    }
    
    /**
     * Clear all elements.
     */
    public void clear() {
        checkClosed();
        nativeClear(nativeHandle);
    }
    
    /**
     * Get number of non-expired elements.
     * @return size
     */
    public int size() {
        checkClosed();
        return nativeSize(nativeHandle);
    }
    
    /**
     * Check if stack is empty.
     * @return true if empty
     */
    public boolean isEmpty() {
        checkClosed();
        return nativeIsEmpty(nativeHandle);
    }
    
    /**
     * Flush changes to disk.
     */
    public void flush() {
        checkClosed();
        nativeFlush(nativeHandle);
    }
    
    /**
     * Get backing file path.
     * @return file path
     */
    public String getFilePath() {
        return filePath;
    }
    
    @Override
    public void close() {
        if (!closed) {
            closed = true;
            nativeDestroy(nativeHandle);
        }
    }
    
    @Override
    public Iterator<T> iterator() {
        throw new UnsupportedOperationException("Iterator not supported");
    }
    
    private void checkClosed() {
        if (closed) throw new IllegalStateException("Stack is closed");
    }
    
    @SuppressWarnings("unchecked")
    private T deserialize(byte[] data) {
        try (ByteArrayInputStream bis = new ByteArrayInputStream(data);
             ObjectInputStream ois = new ObjectInputStream(bis)) {
            return (T) ois.readObject();
        } catch (Exception e) { 
            throw new FastCollectionException("Deserialization failed", e); 
        }
    }
    
    private byte[] serialize(Object obj) {
        try (ByteArrayOutputStream bos = new ByteArrayOutputStream();
             ObjectOutputStream oos = new ObjectOutputStream(bos)) {
            oos.writeObject(obj); 
            return bos.toByteArray();
        } catch (Exception e) { 
            throw new FastCollectionException("Serialization failed", e); 
        }
    }
}
