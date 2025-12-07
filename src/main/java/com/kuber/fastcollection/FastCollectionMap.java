/**
 * Copyright © 2025-2030, All Rights Reserved
 * Ashutosh Sinha | Email: ajsinha@gmail.com
 * 
 * Patent Pending
 */
package com.kuber.fastcollection;

import java.io.*;
import java.util.*;

/**
 * Ultra high-performance memory-mapped key-value map with TTL support.
 * <p>
 * FastCollectionMap provides O(1) average-case operations with automatic
 * persistence and TTL-based expiration for cache-like behavior.
 * 
 * <h2>Key Features</h2>
 * <ul>
 *   <li>Per-entry TTL for automatic expiration</li>
 *   <li>Atomic operations: putIfAbsent, replace</li>
 *   <li>Persistent storage via memory-mapped files</li>
 *   <li>Lock-striped concurrency for high throughput</li>
 * </ul>
 * 
 * <h2>TTL Behavior</h2>
 * <pre>
 * map.put(key, value);           // Never expires (TTL = -1)
 * map.put(key, value, 300);      // Expires in 5 minutes
 * map.setTTL(key, 600);          // Extend TTL to 10 minutes
 * long remaining = map.getTTL(key);  // Check remaining time
 * </pre>
 * 
 * @param <K> key type (must be Serializable)
 * @param <V> value type (must be Serializable)
 * @author Ashutosh Sinha
 * @since 1.0
 */
public class FastCollectionMap<K extends Serializable, V extends Serializable> 
        implements Map<K, V>, Closeable {
    
    /** Constant indicating infinite TTL (entry never expires). */
    public static final int TTL_INFINITE = -1;
    
    /** Default initial size for the memory-mapped file (64 MB). */
    public static final long DEFAULT_INITIAL_SIZE = 64L * 1024 * 1024;
    
    private final long nativeHandle;
    private final String filePath;
    private volatile boolean closed = false;
    
    static { NativeLibraryLoader.load(); }
    
    private static native long nativeCreate(String filePath, long initialSize, boolean createNew);
    private static native void nativeDestroy(long handle);
    private native boolean nativePut(long handle, byte[] key, byte[] value, int ttlSeconds);
    private native boolean nativePutIfAbsent(long handle, byte[] key, byte[] value, int ttlSeconds);
    private native byte[] nativeGet(long handle, byte[] key);
    private native boolean nativeRemove(long handle, byte[] key);
    private native boolean nativeContainsKey(long handle, byte[] key);
    private native long nativeGetTTL(long handle, byte[] key);
    private native boolean nativeSetTTL(long handle, byte[] key, int ttlSeconds);
    private native int nativeRemoveExpired(long handle);
    private native void nativeClear(long handle);
    private native int nativeSize(long handle);
    private native boolean nativeIsEmpty(long handle);
    private native void nativeFlush(long handle);
    
    /**
     * Create or open a map with default settings.
     * 
     * @param filePath path to the memory-mapped file
     */
    public FastCollectionMap(String filePath) {
        this(filePath, DEFAULT_INITIAL_SIZE, false);
    }
    
    /**
     * Create or open a map with custom settings.
     * 
     * @param filePath path to the memory-mapped file
     * @param initialSize initial size of the memory-mapped file in bytes
     * @param createNew if true, create a new file (overwrite existing)
     */
    public FastCollectionMap(String filePath, long initialSize, boolean createNew) {
        this.filePath = filePath;
        this.nativeHandle = nativeCreate(filePath, initialSize, createNew);
        if (this.nativeHandle == 0) {
            throw new FastCollectionException("Failed to create/open map: " + filePath);
        }
    }
    
    /**
     * Put key-value pair with TTL.
     * 
     * @param key the key
     * @param value the value
     * @param ttlSeconds TTL in seconds (-1 for infinite, never expires)
     * @return previous value, or null
     */
    public V put(K key, V value, int ttlSeconds) {
        checkClosed();
        V prev = get(key);
        nativePut(nativeHandle, serialize(key), serialize(value), ttlSeconds);
        return prev;
    }
    
    @Override
    public V put(K key, V value) {
        return put(key, value, TTL_INFINITE);
    }
    
    /**
     * Put if key doesn't exist, with TTL.
     * 
     * @param key the key
     * @param value the value
     * @param ttlSeconds TTL in seconds (-1 for infinite)
     * @return true if added, false if key exists
     */
    public boolean putIfAbsentWithTTL(K key, V value, int ttlSeconds) {
        checkClosed();
        return nativePutIfAbsent(nativeHandle, serialize(key), serialize(value), ttlSeconds);
    }
    
    /**
     * Put if key doesn't exist (Map interface implementation).
     * 
     * @param key the key
     * @param value the value to associate with key
     * @return the previous value associated with key, or null if there was no mapping
     */
    @Override
    public V putIfAbsent(K key, V value) {
        checkClosed();
        V existing = get(key);
        if (existing == null) {
            nativePutIfAbsent(nativeHandle, serialize(key), serialize(value), TTL_INFINITE);
        }
        return existing;
    }
    
    @Override
    @SuppressWarnings("unchecked")
    public V get(Object key) {
        checkClosed();
        if (key == null) return null;
        byte[] data = nativeGet(nativeHandle, serialize((K) key));
        return data != null ? deserialize(data) : null;
    }
    
    @Override
    @SuppressWarnings("unchecked")
    public V remove(Object key) {
        checkClosed();
        V value = get(key);
        nativeRemove(nativeHandle, serialize((K) key));
        return value;
    }
    
    @Override
    @SuppressWarnings("unchecked")
    public boolean containsKey(Object key) {
        checkClosed();
        if (key == null) return false;
        return nativeContainsKey(nativeHandle, serialize((K) key));
    }
    
    /**
     * Get remaining TTL for a key.
     * 
     * @param key the key
     * @return remaining seconds, -1 if infinite, 0 if expired/not found
     */
    @SuppressWarnings("unchecked")
    public long getTTL(K key) {
        checkClosed();
        return nativeGetTTL(nativeHandle, serialize(key));
    }
    
    /**
     * Update TTL for a key.
     * 
     * @param key the key
     * @param ttlSeconds new TTL in seconds (-1 for infinite)
     * @return true if key exists and TTL updated
     */
    public boolean setTTL(K key, int ttlSeconds) {
        checkClosed();
        return nativeSetTTL(nativeHandle, serialize(key), ttlSeconds);
    }
    
    /**
     * Remove all expired entries.
     * 
     * @return number of entries removed
     */
    public int removeExpired() {
        checkClosed();
        return nativeRemoveExpired(nativeHandle);
    }
    
    @Override public void clear() { checkClosed(); nativeClear(nativeHandle); }
    @Override public int size() { checkClosed(); return nativeSize(nativeHandle); }
    @Override public boolean isEmpty() { checkClosed(); return nativeIsEmpty(nativeHandle); }
    
    /** Flush pending changes to disk. */
    public void flush() { checkClosed(); nativeFlush(nativeHandle); }
    
    /**
     * Get the file path for this map.
     * 
     * @return the file path
     */
    public String getFilePath() { return filePath; }
    
    @Override
    public void close() {
        if (!closed) {
            closed = true;
            nativeDestroy(nativeHandle);
        }
    }
    
    private void checkClosed() {
        if (closed) throw new IllegalStateException("Map is closed");
    }
    
    @SuppressWarnings("unchecked")
    private <T> T deserialize(byte[] data) {
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
    
    // Unsupported Map operations
    @Override public boolean containsValue(Object v) { throw new UnsupportedOperationException(); }
    @Override public void putAll(Map<? extends K, ? extends V> m) { m.forEach(this::put); }
    @Override public Set<K> keySet() { throw new UnsupportedOperationException(); }
    @Override public Collection<V> values() { throw new UnsupportedOperationException(); }
    @Override public Set<Entry<K, V>> entrySet() { throw new UnsupportedOperationException(); }
}
