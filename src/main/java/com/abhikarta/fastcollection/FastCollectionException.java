/**
 * Copyright © 2025-2030, All Rights Reserved
 * Ashutosh Sinha | Email: ajsinha@gmail.com
 * 
 * Patent Pending
 */
package com.abhikarta.fastcollection;

/**
 * Exception thrown by FastCollection operations.
 * <p>
 * This exception wraps errors from the native layer including:
 * <ul>
 *   <li>Memory allocation failures</li>
 *   <li>File I/O errors</li>
 *   <li>Invalid operations</li>
 *   <li>TTL-related errors (element expired)</li>
 * </ul>
 * 
 * @author Ashutosh Sinha
 * @since 1.0
 */
public class FastCollectionException extends RuntimeException {
    
    private static final long serialVersionUID = 1L;
    
    /**
     * Creates a new FastCollectionException with the specified message.
     * 
     * @param message the detail message
     */
    public FastCollectionException(String message) {
        super(message);
    }
    
    /**
     * Creates a new FastCollectionException with the specified message and cause.
     * 
     * @param message the detail message
     * @param cause the cause of this exception
     */
    public FastCollectionException(String message, Throwable cause) {
        super(message, cause);
    }
}
