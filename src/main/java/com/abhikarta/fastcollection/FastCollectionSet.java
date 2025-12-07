/**
 * Copyright © 2025-2030, All Rights Reserved
 * Ashutosh Sinha | Email: ajsinha@gmail.com
 * Patent Pending
 */
package com.abhikarta.fastcollection;

import java.io.*;
import java.util.*;

/**
 * Ultra high-performance memory-mapped set with TTL support.
 * Provides O(1) average-case add, remove, and contains operations.
 * 
 * @param <T> element type (must be Serializable)
 */
public class FastCollectionSet<T extends Serializable> implements Set<T>, Closeable {
    
    public static final int TTL_INFINITE = -1;
    public static final long DEFAULT_INITIAL_SIZE = 64L * 1024 * 1024;
    
    private final long nativeHandle;
    private final String filePath;
    private volatile boolean closed = false;
    
    static { NativeLibraryLoader.load(); }
    
    private static native long nativeCreate(String filePath, long initialSize, boolean createNew);
    private static native void nativeDestroy(long handle);
    private native boolean nativeAdd(long handle, byte[] data, int ttlSeconds);
    private native boolean nativeRemove(long handle, byte[] data);
    private native boolean nativeContains(long handle, byte[] data);
    private native long nativeGetTTL(long handle, byte[] data);
    private native boolean nativeSetTTL(long handle, byte[] data, int ttlSeconds);
    private native int nativeRemoveExpired(long handle);
    private native void nativeClear(long handle);
    private native int nativeSize(long handle);
    private native boolean nativeIsEmpty(long handle);
    private native void nativeFlush(long handle);
    
    public FastCollectionSet(String filePath) { this(filePath, DEFAULT_INITIAL_SIZE, false); }
    
    public FastCollectionSet(String filePath, long initialSize, boolean createNew) {
        this.filePath = filePath;
        this.nativeHandle = nativeCreate(filePath, initialSize, createNew);
        if (this.nativeHandle == 0) throw new FastCollectionException("Failed to create set: " + filePath);
    }
    
    public boolean add(T element, int ttlSeconds) {
        checkClosed();
        return nativeAdd(nativeHandle, serialize(element), ttlSeconds);
    }
    
    @Override public boolean add(T element) { return add(element, TTL_INFINITE); }
    
    @Override @SuppressWarnings("unchecked")
    public boolean remove(Object o) {
        checkClosed();
        if (o == null) return false;
        return nativeRemove(nativeHandle, serialize((T) o));
    }
    
    @Override @SuppressWarnings("unchecked")
    public boolean contains(Object o) {
        checkClosed();
        if (o == null) return false;
        return nativeContains(nativeHandle, serialize((T) o));
    }
    
    @SuppressWarnings("unchecked")
    public long getTTL(T element) { checkClosed(); return nativeGetTTL(nativeHandle, serialize(element)); }
    public boolean setTTL(T element, int ttlSeconds) { checkClosed(); return nativeSetTTL(nativeHandle, serialize(element), ttlSeconds); }
    public int removeExpired() { checkClosed(); return nativeRemoveExpired(nativeHandle); }
    
    @Override public void clear() { checkClosed(); nativeClear(nativeHandle); }
    @Override public int size() { checkClosed(); return nativeSize(nativeHandle); }
    @Override public boolean isEmpty() { checkClosed(); return nativeIsEmpty(nativeHandle); }
    public void flush() { checkClosed(); nativeFlush(nativeHandle); }
    public String getFilePath() { return filePath; }
    
    @Override public void close() { if (!closed) { closed = true; nativeDestroy(nativeHandle); } }
    private void checkClosed() { if (closed) throw new IllegalStateException("Set is closed"); }
    
    private byte[] serialize(Object obj) {
        try (ByteArrayOutputStream bos = new ByteArrayOutputStream();
             ObjectOutputStream oos = new ObjectOutputStream(bos)) {
            oos.writeObject(obj); return bos.toByteArray();
        } catch (Exception e) { throw new FastCollectionException("Serialization failed", e); }
    }
    
    @Override public Iterator<T> iterator() { throw new UnsupportedOperationException(); }
    @Override public Object[] toArray() { throw new UnsupportedOperationException(); }
    @Override public <U> U[] toArray(U[] a) { throw new UnsupportedOperationException(); }
    @Override public boolean containsAll(Collection<?> c) { for (Object o : c) if (!contains(o)) return false; return true; }
    @Override public boolean addAll(Collection<? extends T> c) { boolean m = false; for (T e : c) if (add(e)) m = true; return m; }
    @Override public boolean retainAll(Collection<?> c) { throw new UnsupportedOperationException(); }
    @Override public boolean removeAll(Collection<?> c) { boolean m = false; for (Object o : c) if (remove(o)) m = true; return m; }
}
