/**
 * Copyright © 2025-2030, All Rights Reserved
 * Ashutosh Sinha | Email: ajsinha@gmail.com
 * 
 * Patent Pending
 */
package com.abhikarta.fastcollection;

import java.io.*;
import java.nio.file.*;

/**
 * Utility class for loading the FastCollection native library.
 * <p>
 * The library is loaded from the classpath (for JAR deployment) or from
 * the system library path. The loading is done once per JVM.
 * 
 * @author Ashutosh Sinha
 * @since 1.0
 */
public final class NativeLibraryLoader {
    
    private static final String LIBRARY_NAME = "fastcollection";
    private static volatile boolean loaded = false;
    private static final Object lock = new Object();
    
    private NativeLibraryLoader() {}
    
    /**
     * Load the native library.
     * <p>
     * This method is safe to call multiple times - the library will only
     * be loaded once.
     * 
     * @throws UnsatisfiedLinkError if the library cannot be loaded
     */
    public static void load() {
        if (loaded) return;
        
        synchronized (lock) {
            if (loaded) return;
            
            try {
                // Try system library path first
                System.loadLibrary(LIBRARY_NAME);
                loaded = true;
                return;
            } catch (UnsatisfiedLinkError e) {
                // Fall through to try extracting from JAR
            }
            
            // Try to extract from JAR
            try {
                Path tempLib = extractLibraryFromJar();
                System.load(tempLib.toAbsolutePath().toString());
                loaded = true;
            } catch (IOException e) {
                throw new UnsatisfiedLinkError(
                    "Failed to load native library: " + e.getMessage()
                );
            }
        }
    }
    
    private static Path extractLibraryFromJar() throws IOException {
        String osName = System.getProperty("os.name").toLowerCase();
        String osArch = System.getProperty("os.arch").toLowerCase();
        
        String libName;
        if (osName.contains("linux")) {
            libName = "lib" + LIBRARY_NAME + ".so";
        } else if (osName.contains("mac") || osName.contains("darwin")) {
            libName = "lib" + LIBRARY_NAME + ".dylib";
        } else if (osName.contains("win")) {
            libName = LIBRARY_NAME + ".dll";
        } else {
            throw new IOException("Unsupported OS: " + osName);
        }
        
        String resourcePath = "/native/" + osName + "/" + osArch + "/" + libName;
        
        try (InputStream in = NativeLibraryLoader.class.getResourceAsStream(resourcePath)) {
            if (in == null) {
                throw new IOException("Native library not found in JAR: " + resourcePath);
            }
            
            Path tempDir = Files.createTempDirectory("fastcollection");
            Path tempLib = tempDir.resolve(libName);
            
            Files.copy(in, tempLib, StandardCopyOption.REPLACE_EXISTING);
            tempLib.toFile().deleteOnExit();
            tempDir.toFile().deleteOnExit();
            
            return tempLib;
        }
    }
    
    /**
     * Check if the native library is loaded.
     * 
     * @return true if loaded
     */
    public static boolean isLoaded() {
        return loaded;
    }
}
