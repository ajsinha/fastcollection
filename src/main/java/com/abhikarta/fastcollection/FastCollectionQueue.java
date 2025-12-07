/**
 * Copyright © 2025-2030, All Rights Reserved
 * Ashutosh Sinha | Email: ajsinha@gmail.com
 * Patent Pending
 */
package com.abhikarta.fastcollection;

import java.io.*;
import java.util.*;

/**
 * Ultra high-performance memory-mapped queue with TTL support.
 * <p>
 * Supports both FIFO queue and deque operations with automatic element expiration.
 * Expired elements are automatically skipped during poll/take operations.
 * 
 * <h2>TTL Behavior</h2>
 * <pre>
 * queue.offer(task);              // Never expires
 * queue.offer(task, 300);         // Expires in 5 minutes
 * queue.poll();                   // Returns next non-expired element
 * </pre>
 * 
 * @param <T> element type (must be Serializable)
 */
public class FastCollectionQueue<T extends Serializable> implements Queue<T>, Deque<T>, Closeable {
    
    public static final int TTL_INFINITE = -1;
    public static final long DEFAULT_INITIAL_SIZE = 64L * 1024 * 1024;
    
    private final long nativeHandle;
    private final String filePath;
    private volatile boolean closed = false;
    
    static { NativeLibraryLoader.load(); }
    
    private static native long nativeCreate(String filePath, long initialSize, boolean createNew);
    private static native void nativeDestroy(long handle);
    private native boolean nativeOffer(long handle, byte[] data, int ttlSeconds);
    private native boolean nativeOfferFirst(long handle, byte[] data, int ttlSeconds);
    private native byte[] nativePoll(long handle);
    private native byte[] nativePollLast(long handle);
    private native byte[] nativePeek(long handle);
    private native long nativePeekTTL(long handle);
    private native int nativeRemoveExpired(long handle);
    private native void nativeClear(long handle);
    private native int nativeSize(long handle);
    private native boolean nativeIsEmpty(long handle);
    private native void nativeFlush(long handle);
    
    public FastCollectionQueue(String filePath) { this(filePath, DEFAULT_INITIAL_SIZE, false); }
    
    public FastCollectionQueue(String filePath, long initialSize, boolean createNew) {
        this.filePath = filePath;
        this.nativeHandle = nativeCreate(filePath, initialSize, createNew);
        if (this.nativeHandle == 0) throw new FastCollectionException("Failed to create queue: " + filePath);
    }
    
    /**
     * Add element to tail with TTL.
     * @param element the element
     * @param ttlSeconds TTL in seconds (-1 for infinite)
     * @return true if successful
     */
    public boolean offer(T element, int ttlSeconds) {
        checkClosed();
        return nativeOffer(nativeHandle, serialize(element), ttlSeconds);
    }
    
    @Override public boolean offer(T element) { return offer(element, TTL_INFINITE); }
    @Override public boolean add(T element) { return offer(element); }
    
    /**
     * Add element to front with TTL.
     */
    public boolean offerFirst(T element, int ttlSeconds) {
        checkClosed();
        return nativeOfferFirst(nativeHandle, serialize(element), ttlSeconds);
    }
    
    @Override public boolean offerFirst(T element) { return offerFirst(element, TTL_INFINITE); }
    @Override public boolean offerLast(T element) { return offer(element); }
    @Override public void addFirst(T element) { offerFirst(element); }
    @Override public void addLast(T element) { offer(element); }
    
    @Override public T poll() { checkClosed(); byte[] data = nativePoll(nativeHandle); return data != null ? deserialize(data) : null; }
    @Override public T pollFirst() { return poll(); }
    @Override public T pollLast() { checkClosed(); byte[] data = nativePollLast(nativeHandle); return data != null ? deserialize(data) : null; }
    @Override public T peek() { checkClosed(); byte[] data = nativePeek(nativeHandle); return data != null ? deserialize(data) : null; }
    @Override public T peekFirst() { return peek(); }
    @Override public T peekLast() { return peek(); } // Simplified
    
    @Override public T remove() { T e = poll(); if (e == null) throw new NoSuchElementException(); return e; }
    @Override public T removeFirst() { return remove(); }
    @Override public T removeLast() { T e = pollLast(); if (e == null) throw new NoSuchElementException(); return e; }
    @Override public T element() { T e = peek(); if (e == null) throw new NoSuchElementException(); return e; }
    @Override public T getFirst() { return element(); }
    @Override public T getLast() { return element(); } // Simplified
    
    @Override public void push(T e) { addFirst(e); }
    @Override public T pop() { return removeFirst(); }
    
    public long peekTTL() { checkClosed(); return nativePeekTTL(nativeHandle); }
    public int removeExpired() { checkClosed(); return nativeRemoveExpired(nativeHandle); }
    
    @Override public void clear() { checkClosed(); nativeClear(nativeHandle); }
    @Override public int size() { checkClosed(); return nativeSize(nativeHandle); }
    @Override public boolean isEmpty() { checkClosed(); return nativeIsEmpty(nativeHandle); }
    public void flush() { checkClosed(); nativeFlush(nativeHandle); }
    public String getFilePath() { return filePath; }
    
    @Override public void close() { if (!closed) { closed = true; nativeDestroy(nativeHandle); } }
    private void checkClosed() { if (closed) throw new IllegalStateException("Queue is closed"); }
    
    @SuppressWarnings("unchecked")
    private T deserialize(byte[] data) {
        try (ByteArrayInputStream bis = new ByteArrayInputStream(data);
             ObjectInputStream ois = new ObjectInputStream(bis)) {
            return (T) ois.readObject();
        } catch (Exception e) { throw new FastCollectionException("Deserialization failed", e); }
    }
    
    private byte[] serialize(Object obj) {
        try (ByteArrayOutputStream bos = new ByteArrayOutputStream();
             ObjectOutputStream oos = new ObjectOutputStream(bos)) {
            oos.writeObject(obj); return bos.toByteArray();
        } catch (Exception e) { throw new FastCollectionException("Serialization failed", e); }
    }
    
    @Override public boolean removeFirstOccurrence(Object o) { throw new UnsupportedOperationException(); }
    @Override public boolean removeLastOccurrence(Object o) { throw new UnsupportedOperationException(); }
    @Override public boolean contains(Object o) { throw new UnsupportedOperationException(); }
    @Override public boolean remove(Object o) { throw new UnsupportedOperationException(); }
    @Override public Iterator<T> iterator() { throw new UnsupportedOperationException(); }
    @Override public Iterator<T> descendingIterator() { throw new UnsupportedOperationException(); }
    @Override public Object[] toArray() { throw new UnsupportedOperationException(); }
    @Override public <U> U[] toArray(U[] a) { throw new UnsupportedOperationException(); }
    @Override public boolean containsAll(Collection<?> c) { throw new UnsupportedOperationException(); }
    @Override public boolean addAll(Collection<? extends T> c) { boolean m = false; for (T e : c) if (add(e)) m = true; return m; }
    @Override public boolean removeAll(Collection<?> c) { throw new UnsupportedOperationException(); }
    @Override public boolean retainAll(Collection<?> c) { throw new UnsupportedOperationException(); }
}
