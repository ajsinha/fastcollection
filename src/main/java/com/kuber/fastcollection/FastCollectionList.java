/**
 * Copyright © 2025-2030, All Rights Reserved
 * Ashutosh Sinha | Email: ajsinha@gmail.com
 * 
 * Patent Pending
 */
package com.kuber.fastcollection;

import java.io.*;
import java.util.*;
import java.util.function.*;

/**
 * Ultra high-performance memory-mapped list with TTL (Time-To-Live) support.
 * <p>
 * FastCollectionList stores serialized Java objects in a memory-mapped file,
 * providing automatic persistence and inter-process communication (IPC).
 * Elements can optionally be assigned a TTL for automatic expiration.
 * 
 * <h2>Key Features</h2>
 * <ul>
 *   <li><b>Persistence:</b> Data survives JVM restarts automatically</li>
 *   <li><b>IPC:</b> Multiple JVMs can share the same list</li>
 *   <li><b>TTL:</b> Elements can auto-expire after a configurable duration</li>
 *   <li><b>Performance:</b> &lt;500ns average add/remove operations</li>
 * </ul>
 * 
 * <h2>TTL Behavior</h2>
 * <pre>
 * // TTL of -1 means the element never expires (default)
 * list.add(myObject);                    // Never expires
 * list.add(myObject, -1);                // Never expires (explicit)
 * 
 * // TTL &gt; 0 means element expires after specified seconds
 * list.add(myObject, 300);               // Expires in 5 minutes
 * list.add(myObject, 3600);              // Expires in 1 hour
 * 
 * // Check remaining TTL
 * long remaining = list.getTTL(0);       // Seconds remaining, -1 if infinite
 * 
 * // Update TTL
 * list.setTTL(0, 600);                   // Extend to 10 minutes
 * </pre>
 * 
 * <h2>Usage Example</h2>
 * <pre>
 * // Create or open a list
 * try (FastCollectionList&lt;MyObject&gt; list = 
 *         new FastCollectionList&lt;&gt;("/tmp/mylist.fc", MyObject.class)) {
 *     
 *     // Add with 5-minute TTL
 *     list.add(new MyObject("data"), 300);
 *     
 *     // Add without TTL (never expires)
 *     list.add(new MyObject("permanent"));
 *     
 *     // Get element
 *     MyObject obj = list.get(0);
 *     
 *     // Remove expired elements
 *     int removed = list.removeExpired();
 * }
 * </pre>
 * 
 * @param <T> the type of elements in this list (must be Serializable)
 * @author Ashutosh Sinha
 * @since 1.0
 */
public class FastCollectionList<T extends Serializable> implements List<T>, Closeable {
    
    /** Default TTL value indicating no expiration */
    public static final int TTL_INFINITE = -1;
    
    /** Default initial size: 64MB */
    public static final long DEFAULT_INITIAL_SIZE = 64L * 1024 * 1024;
    
    private final long nativeHandle;
    private final Class<T> elementType;
    private final String filePath;
    private volatile boolean closed = false;
    
    static {
        NativeLibraryLoader.load();
    }
    
    // Native methods
    private static native long nativeCreate(String filePath, long initialSize, boolean createNew);
    private static native void nativeDestroy(long handle);
    private native boolean nativeAdd(long handle, byte[] data, int ttlSeconds);
    private native boolean nativeAddAt(long handle, int index, byte[] data, int ttlSeconds);
    private native boolean nativeAddFirst(long handle, byte[] data, int ttlSeconds);
    private native byte[] nativeGet(long handle, int index);
    private native byte[] nativeGetFirst(long handle);
    private native byte[] nativeGetLast(long handle);
    private native boolean nativeSet(long handle, int index, byte[] data, int ttlSeconds);
    private native byte[] nativeRemove(long handle, int index);
    private native byte[] nativeRemoveFirst(long handle);
    private native byte[] nativeRemoveLast(long handle);
    private native boolean nativeContains(long handle, byte[] data);
    private native int nativeIndexOf(long handle, byte[] data);
    private native long nativeGetTTL(long handle, int index);
    private native boolean nativeSetTTL(long handle, int index, int ttlSeconds);
    private native int nativeRemoveExpired(long handle);
    private native void nativeClear(long handle);
    private native int nativeSize(long handle);
    private native boolean nativeIsEmpty(long handle);
    private native void nativeFlush(long handle);
    
    /**
     * Creates a new FastCollectionList with the specified memory-mapped file.
     * <p>
     * If the file exists, it will be opened and validated. If it doesn't exist,
     * a new file will be created.
     * 
     * @param filePath path to the memory-mapped file
     * @param elementType class of elements stored in the list
     * @throws FastCollectionException if file cannot be created or opened
     */
    public FastCollectionList(String filePath, Class<T> elementType) {
        this(filePath, elementType, DEFAULT_INITIAL_SIZE, false);
    }
    
    /**
     * Creates a new FastCollectionList with specified size.
     * 
     * @param filePath path to the memory-mapped file
     * @param elementType class of elements stored in the list
     * @param initialSize initial size in bytes (file will grow automatically)
     * @param createNew if true, truncate existing file
     * @throws FastCollectionException if file cannot be created or opened
     */
    public FastCollectionList(String filePath, Class<T> elementType, 
                               long initialSize, boolean createNew) {
        this.filePath = filePath;
        this.elementType = elementType;
        this.nativeHandle = nativeCreate(filePath, initialSize, createNew);
        
        if (this.nativeHandle == 0) {
            throw new FastCollectionException("Failed to create/open collection: " + filePath);
        }
    }
    
    /**
     * Add an element with TTL.
     * 
     * @param element the element to add
     * @param ttlSeconds time-to-live in seconds (-1 for infinite, never expires)
     * @return true if successful
     */
    public boolean add(T element, int ttlSeconds) {
        checkClosed();
        byte[] data = serialize(element);
        return nativeAdd(nativeHandle, data, ttlSeconds);
    }
    
    @Override
    public boolean add(T element) {
        return add(element, TTL_INFINITE);
    }
    
    /**
     * Add an element at index with TTL.
     * 
     * @param index position to insert at
     * @param element the element to add
     * @param ttlSeconds time-to-live in seconds (-1 for infinite)
     */
    public void add(int index, T element, int ttlSeconds) {
        checkClosed();
        byte[] data = serialize(element);
        if (!nativeAddAt(nativeHandle, index, data, ttlSeconds)) {
            throw new IndexOutOfBoundsException("Index: " + index);
        }
    }
    
    @Override
    public void add(int index, T element) {
        add(index, element, TTL_INFINITE);
    }
    
    /**
     * Add element at the front with TTL.
     * 
     * @param element the element to add
     * @param ttlSeconds time-to-live in seconds (-1 for infinite)
     * @return true if successful
     */
    public boolean addFirst(T element, int ttlSeconds) {
        checkClosed();
        byte[] data = serialize(element);
        return nativeAddFirst(nativeHandle, data, ttlSeconds);
    }
    
    /**
     * Add element at the front without TTL (never expires).
     * 
     * @param element the element to add
     * @return true if successful
     */
    public boolean addFirst(T element) {
        return addFirst(element, TTL_INFINITE);
    }
    
    @Override
    @SuppressWarnings("unchecked")
    public T get(int index) {
        checkClosed();
        byte[] data = nativeGet(nativeHandle, index);
        if (data == null) {
            throw new IndexOutOfBoundsException("Index: " + index + " or element expired");
        }
        return deserialize(data);
    }
    
    /**
     * Get the first element.
     * 
     * @return the first element, or null if empty or expired
     */
    @SuppressWarnings("unchecked")
    public T getFirst() {
        checkClosed();
        byte[] data = nativeGetFirst(nativeHandle);
        return data != null ? deserialize(data) : null;
    }
    
    /**
     * Get the last element.
     * 
     * @return the last element, or null if empty or expired
     */
    @SuppressWarnings("unchecked")
    public T getLast() {
        checkClosed();
        byte[] data = nativeGetLast(nativeHandle);
        return data != null ? deserialize(data) : null;
    }
    
    /**
     * Set element at index with TTL.
     * 
     * @param index position
     * @param element new element
     * @param ttlSeconds new TTL in seconds (-1 for infinite)
     * @return the previous element
     */
    public T set(int index, T element, int ttlSeconds) {
        checkClosed();
        T previous = get(index);
        byte[] data = serialize(element);
        if (!nativeSet(nativeHandle, index, data, ttlSeconds)) {
            throw new IndexOutOfBoundsException("Index: " + index);
        }
        return previous;
    }
    
    @Override
    public T set(int index, T element) {
        return set(index, element, TTL_INFINITE);
    }
    
    @Override
    public T remove(int index) {
        checkClosed();
        byte[] data = nativeRemove(nativeHandle, index);
        if (data == null) {
            throw new IndexOutOfBoundsException("Index: " + index);
        }
        return deserialize(data);
    }
    
    /**
     * Remove and return the first element.
     * 
     * @return the removed element, or null if empty
     */
    public T removeFirst() {
        checkClosed();
        byte[] data = nativeRemoveFirst(nativeHandle);
        return data != null ? deserialize(data) : null;
    }
    
    /**
     * Remove and return the last element.
     * 
     * @return the removed element, or null if empty
     */
    public T removeLast() {
        checkClosed();
        byte[] data = nativeRemoveLast(nativeHandle);
        return data != null ? deserialize(data) : null;
    }
    
    @Override
    @SuppressWarnings("unchecked")
    public boolean remove(Object o) {
        checkClosed();
        int idx = indexOf(o);
        if (idx >= 0) {
            remove(idx);
            return true;
        }
        return false;
    }
    
    @Override
    public boolean contains(Object o) {
        checkClosed();
        if (o == null || !elementType.isInstance(o)) return false;
        byte[] data = serialize((T) o);
        return nativeContains(nativeHandle, data);
    }
    
    @Override
    @SuppressWarnings("unchecked")
    public int indexOf(Object o) {
        checkClosed();
        if (o == null || !elementType.isInstance(o)) return -1;
        byte[] data = serialize((T) o);
        return nativeIndexOf(nativeHandle, data);
    }
    
    @Override
    public int lastIndexOf(Object o) {
        // For simplicity, use forward scan
        return indexOf(o);
    }
    
    /**
     * Get remaining TTL for element at index.
     * 
     * @param index position
     * @return remaining seconds, -1 if infinite (no expiration), 0 if expired
     */
    public long getTTL(int index) {
        checkClosed();
        return nativeGetTTL(nativeHandle, index);
    }
    
    /**
     * Update TTL for element at index.
     * 
     * @param index position
     * @param ttlSeconds new TTL in seconds (-1 for infinite)
     * @return true if successful
     */
    public boolean setTTL(int index, int ttlSeconds) {
        checkClosed();
        return nativeSetTTL(nativeHandle, index, ttlSeconds);
    }
    
    /**
     * Remove all expired elements.
     * 
     * @return number of elements removed
     */
    public int removeExpired() {
        checkClosed();
        return nativeRemoveExpired(nativeHandle);
    }
    
    @Override
    public void clear() {
        checkClosed();
        nativeClear(nativeHandle);
    }
    
    @Override
    public int size() {
        checkClosed();
        return nativeSize(nativeHandle);
    }
    
    @Override
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
     * Get the backing file path.
     * 
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
    
    private void checkClosed() {
        if (closed) {
            throw new IllegalStateException("Collection is closed");
        }
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
    
    private byte[] serialize(T obj) {
        try (ByteArrayOutputStream bos = new ByteArrayOutputStream();
             ObjectOutputStream oos = new ObjectOutputStream(bos)) {
            oos.writeObject(obj);
            return bos.toByteArray();
        } catch (Exception e) {
            throw new FastCollectionException("Serialization failed", e);
        }
    }
    
    // ========================================================================
    // List interface methods - simplified implementations
    // ========================================================================
    
    @Override
    public boolean addAll(Collection<? extends T> c) {
        boolean modified = false;
        for (T element : c) {
            if (add(element)) modified = true;
        }
        return modified;
    }
    
    @Override
    public boolean addAll(int index, Collection<? extends T> c) {
        int i = index;
        for (T element : c) {
            add(i++, element);
        }
        return !c.isEmpty();
    }
    
    @Override
    public boolean containsAll(Collection<?> c) {
        for (Object o : c) {
            if (!contains(o)) return false;
        }
        return true;
    }
    
    @Override
    public boolean removeAll(Collection<?> c) {
        boolean modified = false;
        for (Object o : c) {
            if (remove(o)) modified = true;
        }
        return modified;
    }
    
    @Override
    public boolean retainAll(Collection<?> c) {
        throw new UnsupportedOperationException("retainAll not supported");
    }
    
    @Override
    public Object[] toArray() {
        int sz = size();
        Object[] result = new Object[sz];
        for (int i = 0; i < sz; i++) {
            result[i] = get(i);
        }
        return result;
    }
    
    @Override
    @SuppressWarnings("unchecked")
    public <U> U[] toArray(U[] a) {
        int sz = size();
        if (a.length < sz) {
            a = (U[]) java.lang.reflect.Array.newInstance(a.getClass().getComponentType(), sz);
        }
        for (int i = 0; i < sz; i++) {
            a[i] = (U) get(i);
        }
        if (a.length > sz) a[sz] = null;
        return a;
    }
    
    @Override
    public Iterator<T> iterator() {
        return new FastListIterator(0);
    }
    
    @Override
    public ListIterator<T> listIterator() {
        return new FastListIterator(0);
    }
    
    @Override
    public ListIterator<T> listIterator(int index) {
        return new FastListIterator(index);
    }
    
    @Override
    public List<T> subList(int fromIndex, int toIndex) {
        throw new UnsupportedOperationException("subList not supported");
    }
    
    private class FastListIterator implements ListIterator<T> {
        private int cursor;
        private int lastRet = -1;
        
        FastListIterator(int index) {
            this.cursor = index;
        }
        
        @Override public boolean hasNext() { return cursor < size(); }
        @Override public T next() { lastRet = cursor; return get(cursor++); }
        @Override public boolean hasPrevious() { return cursor > 0; }
        @Override public T previous() { lastRet = --cursor; return get(cursor); }
        @Override public int nextIndex() { return cursor; }
        @Override public int previousIndex() { return cursor - 1; }
        @Override public void remove() { FastCollectionList.this.remove(lastRet); cursor = lastRet; }
        @Override public void set(T e) { FastCollectionList.this.set(lastRet, e); }
        @Override public void add(T e) { FastCollectionList.this.add(cursor++, e); }
    }
}
