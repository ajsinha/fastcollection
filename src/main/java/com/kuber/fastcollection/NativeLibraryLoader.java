/**
 * Copyright © 2025-2030, All Rights Reserved
 * Ashutosh Sinha | Email: ajsinha@gmail.com
 * 
 * Patent Pending
 */
package com.kuber.fastcollection;

import java.io.*;
import java.nio.file.*;
import java.util.*;
import java.util.logging.*;

/**
 * Cross-platform native library loader for FastCollection.
 * <p>
 * Automatically detects the operating system and architecture, then loads
 * the appropriate native library. The library is bundled inside the JAR
 * and extracted to a temporary location on first use.
 * 
 * <h2>Supported Platforms</h2>
 * <ul>
 *   <li>Windows (x64, x86, arm64)</li>
 *   <li>Linux (x64, x86, arm64, arm)</li>
 *   <li>macOS (x64, arm64)</li>
 * </ul>
 * 
 * <h2>Loading Strategy</h2>
 * <ol>
 *   <li>Check if already loaded</li>
 *   <li>Try java.library.path (for development/custom deployments)</li>
 *   <li>Try extracting platform-specific library from JAR resources</li>
 *   <li>Try legacy library name as fallback</li>
 * </ol>
 * 
 * @author Ashutosh Sinha
 * @since 1.0
 */
public final class NativeLibraryLoader {
    
    private static final Logger LOGGER = Logger.getLogger(NativeLibraryLoader.class.getName());
    
    /** Base library name without platform suffix */
    private static final String LIBRARY_BASE_NAME = "fastcollection";
    
    /** Library version for cache invalidation */
    private static final String LIBRARY_VERSION = "1.0.0";
    
    /** Flag indicating if library is loaded */
    private static volatile boolean loaded = false;
    
    /** Lock for thread-safe loading */
    private static final Object LOAD_LOCK = new Object();
    
    /** Detected platform info */
    private static final Platform PLATFORM = Platform.detect();
    
    /** Temporary directory for extracted libraries */
    private static Path extractedLibraryPath = null;
    
    // Prevent instantiation
    private NativeLibraryLoader() {}
    
    /**
     * Operating system enumeration with library naming conventions.
     */
    public enum OS {
        /** Microsoft Windows operating system. */
        WINDOWS("windows", "", ".dll"),
        /** Linux operating system. */
        LINUX("linux", "lib", ".so"),
        /** macOS (Apple) operating system. */
        MACOS("macos", "lib", ".dylib"),
        /** Unknown or unsupported operating system. */
        UNKNOWN("unknown", "", "");
        
        final String name;
        final String prefix;
        final String suffix;
        
        OS(String name, String prefix, String suffix) {
            this.name = name;
            this.prefix = prefix;
            this.suffix = suffix;
        }
    }
    
    /**
     * CPU architecture enumeration.
     */
    public enum Arch {
        /** 64-bit x86 architecture (AMD64/Intel 64). */
        X64("x64"),
        /** 32-bit x86 architecture. */
        X86("x86"),
        /** 64-bit ARM architecture (AArch64). */
        ARM64("arm64"),
        /** 32-bit ARM architecture. */
        ARM("arm"),
        /** Unknown or unsupported architecture. */
        UNKNOWN("unknown");
        
        final String name;
        
        Arch(String name) {
            this.name = name;
        }
    }
    
    /**
     * Platform detection result containing OS and architecture information.
     */
    public static class Platform {
        /** The detected operating system. */
        public final OS os;
        /** The detected CPU architecture. */
        public final Arch arch;
        
        private Platform(OS os, Arch arch) {
            this.os = os;
            this.arch = arch;
        }
        
        /**
         * Get the platform-specific library name.
         * Format: {@code fastcollection-<os>-<arch>.<ext>}
         * 
         * @return the platform-specific library filename
         */
        public String getLibraryName() {
            return LIBRARY_BASE_NAME + "-" + os.name + "-" + arch.name + os.suffix;
        }
        
        /**
         * Get the legacy library name (without platform suffix).
         * 
         * @return the legacy library filename
         */
        public String getLegacyLibraryName() {
            return os.prefix + LIBRARY_BASE_NAME + os.suffix;
        }
        
        /**
         * Get the resource path inside the JAR.
         * 
         * @return the resource path for the native library
         */
        public String getResourcePath() {
            return "/native/" + os.name + "/" + arch.name + "/" + getLibraryName();
        }
        
        /**
         * Get alternative resource paths to try.
         * 
         * @return list of alternative resource paths
         */
        public List<String> getAlternativeResourcePaths() {
            List<String> paths = new ArrayList<>();
            // Primary path with platform-specific name
            paths.add("/native/" + os.name + "/" + arch.name + "/" + getLibraryName());
            // Legacy path with generic name
            paths.add("/native/" + os.name + "/" + arch.name + "/" + getLegacyLibraryName());
            // Flat structure (no arch subdirectory)
            paths.add("/native/" + os.name + "/" + getLibraryName());
            paths.add("/native/" + getLibraryName());
            return paths;
        }
        
        @Override
        public String toString() {
            return os.name + "-" + arch.name;
        }
        
        /**
         * Detect the current platform.
         * 
         * @return the detected platform information
         */
        public static Platform detect() {
            OS os = detectOS();
            Arch arch = detectArch();
            return new Platform(os, arch);
        }
        
        private static OS detectOS() {
            String osName = System.getProperty("os.name", "").toLowerCase(Locale.ROOT);
            
            if (osName.contains("win")) {
                return OS.WINDOWS;
            } else if (osName.contains("mac") || osName.contains("darwin")) {
                return OS.MACOS;
            } else if (osName.contains("linux") || osName.contains("unix") || 
                       osName.contains("freebsd") || osName.contains("sunos")) {
                return OS.LINUX;
            }
            return OS.UNKNOWN;
        }
        
        private static Arch detectArch() {
            String osArch = System.getProperty("os.arch", "").toLowerCase(Locale.ROOT);
            
            // 64-bit x86
            if (osArch.equals("amd64") || osArch.equals("x86_64") || osArch.equals("x64")) {
                return Arch.X64;
            }
            // 32-bit x86
            if (osArch.equals("x86") || osArch.equals("i386") || osArch.equals("i486") ||
                osArch.equals("i586") || osArch.equals("i686")) {
                return Arch.X86;
            }
            // 64-bit ARM
            if (osArch.equals("aarch64") || osArch.equals("arm64")) {
                return Arch.ARM64;
            }
            // 32-bit ARM
            if (osArch.startsWith("arm")) {
                return Arch.ARM;
            }
            return Arch.UNKNOWN;
        }
    }
    
    /**
     * Load the native library.
     * <p>
     * This method is thread-safe and idempotent - calling it multiple times
     * will only load the library once.
     * 
     * @throws UnsatisfiedLinkError if the library cannot be loaded
     */
    public static void load() {
        if (loaded) {
            return;
        }
        
        synchronized (LOAD_LOCK) {
            if (loaded) {
                return;
            }
            
            List<String> errors = new ArrayList<>();
            
            // Strategy 1: Try java.library.path with platform-specific name
            if (tryLoadFromLibraryPath(PLATFORM.getLibraryName(), errors)) {
                return;
            }
            
            // Strategy 2: Try java.library.path with legacy name
            if (tryLoadFromLibraryPath(LIBRARY_BASE_NAME, errors)) {
                return;
            }
            
            // Strategy 3: Extract from JAR resources
            if (tryLoadFromResources(errors)) {
                return;
            }
            
            // Strategy 4: Try System.loadLibrary as last resort
            if (trySystemLoadLibrary(errors)) {
                return;
            }
            
            // All strategies failed
            throw new UnsatisfiedLinkError(buildErrorMessage(errors));
        }
    }
    
    /**
     * Try to load library from java.library.path.
     */
    private static boolean tryLoadFromLibraryPath(String libName, List<String> errors) {
        try {
            String libraryPath = System.getProperty("java.library.path", "");
            for (String path : libraryPath.split(File.pathSeparator)) {
                if (path.isEmpty()) continue;
                
                File libFile = new File(path, PLATFORM.os.prefix + libName + PLATFORM.os.suffix);
                if (libFile.exists() && libFile.isFile()) {
                    System.load(libFile.getAbsolutePath());
                    loaded = true;
                    LOGGER.info("Loaded native library from: " + libFile.getAbsolutePath());
                    return true;
                }
            }
            errors.add("Library not found in java.library.path: " + libName);
        } catch (UnsatisfiedLinkError e) {
            errors.add("Failed to load from library path: " + e.getMessage());
        }
        return false;
    }
    
    /**
     * Try to load library from JAR resources.
     */
    private static boolean tryLoadFromResources(List<String> errors) {
        for (String resourcePath : PLATFORM.getAlternativeResourcePaths()) {
            try (InputStream in = NativeLibraryLoader.class.getResourceAsStream(resourcePath)) {
                if (in == null) {
                    continue;
                }
                
                Path tempLib = extractLibrary(in, resourcePath);
                System.load(tempLib.toAbsolutePath().toString());
                loaded = true;
                extractedLibraryPath = tempLib;
                LOGGER.info("Loaded native library from JAR: " + resourcePath);
                return true;
                
            } catch (IOException e) {
                errors.add("Failed to extract from " + resourcePath + ": " + e.getMessage());
            } catch (UnsatisfiedLinkError e) {
                errors.add("Failed to load extracted library: " + e.getMessage());
            }
        }
        return false;
    }
    
    /**
     * Try System.loadLibrary as last resort.
     */
    private static boolean trySystemLoadLibrary(List<String> errors) {
        try {
            System.loadLibrary(LIBRARY_BASE_NAME);
            loaded = true;
            LOGGER.info("Loaded native library via System.loadLibrary");
            return true;
        } catch (UnsatisfiedLinkError e) {
            errors.add("System.loadLibrary failed: " + e.getMessage());
        }
        return false;
    }
    
    /**
     * Extract library from input stream to temporary file.
     */
    private static Path extractLibrary(InputStream in, String resourcePath) throws IOException {
        // Create temp directory with version to handle upgrades
        String tempDirName = LIBRARY_BASE_NAME + "-" + LIBRARY_VERSION + "-" + PLATFORM;
        Path tempDir = Paths.get(System.getProperty("java.io.tmpdir"), tempDirName);
        
        if (!Files.exists(tempDir)) {
            Files.createDirectories(tempDir);
        }
        
        // Extract filename from resource path
        String fileName = resourcePath.substring(resourcePath.lastIndexOf('/') + 1);
        Path tempLib = tempDir.resolve(fileName);
        
        // Check if already extracted (cache)
        if (Files.exists(tempLib)) {
            LOGGER.fine("Using cached library: " + tempLib);
            return tempLib;
        }
        
        // Extract to temp file first, then move (atomic on same filesystem)
        Path tempFile = Files.createTempFile(tempDir, "fc_", ".tmp");
        try {
            Files.copy(in, tempFile, StandardCopyOption.REPLACE_EXISTING);
            
            // Make executable on Unix-like systems
            if (PLATFORM.os != OS.WINDOWS) {
                tempFile.toFile().setExecutable(true);
            }
            
            // Move to final location
            Files.move(tempFile, tempLib, StandardCopyOption.REPLACE_EXISTING);
            
        } catch (IOException e) {
            Files.deleteIfExists(tempFile);
            throw e;
        }
        
        // Schedule cleanup on JVM exit
        tempLib.toFile().deleteOnExit();
        
        LOGGER.fine("Extracted library to: " + tempLib);
        return tempLib;
    }
    
    /**
     * Build comprehensive error message.
     */
    private static String buildErrorMessage(List<String> errors) {
        StringBuilder sb = new StringBuilder();
        sb.append("Failed to load FastCollection native library for platform: ");
        sb.append(PLATFORM).append("\n");
        sb.append("OS: ").append(System.getProperty("os.name")).append("\n");
        sb.append("Arch: ").append(System.getProperty("os.arch")).append("\n");
        sb.append("java.library.path: ").append(System.getProperty("java.library.path")).append("\n");
        sb.append("\nAttempted loading strategies:\n");
        for (int i = 0; i < errors.size(); i++) {
            sb.append("  ").append(i + 1).append(". ").append(errors.get(i)).append("\n");
        }
        sb.append("\nPlease ensure the native library is available:\n");
        sb.append("  - Include fastcollection JAR with native libraries, or\n");
        sb.append("  - Set java.library.path to directory containing the library, or\n");
        sb.append("  - Place library in system library path\n");
        return sb.toString();
    }
    
    /**
     * Check if the native library is loaded.
     * 
     * @return true if the library has been successfully loaded
     */
    public static boolean isLoaded() {
        return loaded;
    }
    
    /**
     * Get the detected platform.
     * 
     * @return detected platform information
     */
    public static Platform getPlatform() {
        return PLATFORM;
    }
    
    /**
     * Get the path to the extracted library, if any.
     * 
     * @return path to extracted library, or null if loaded from system path
     */
    public static Path getExtractedLibraryPath() {
        return extractedLibraryPath;
    }
    
    /**
     * Get the expected library name for the current platform.
     * 
     * @return platform-specific library name
     */
    public static String getExpectedLibraryName() {
        return PLATFORM.getLibraryName();
    }
    
    /**
     * Manually set a custom library path.
     * Must be called before any FastCollection class is used.
     * 
     * @param libraryPath absolute path to the native library file
     * @throws UnsatisfiedLinkError if the library cannot be loaded
     * @throws IllegalStateException if a library is already loaded
     */
    public static void loadFrom(String libraryPath) {
        synchronized (LOAD_LOCK) {
            if (loaded) {
                throw new IllegalStateException("Native library already loaded");
            }
            
            File libFile = new File(libraryPath);
            if (!libFile.exists()) {
                throw new UnsatisfiedLinkError("Library file not found: " + libraryPath);
            }
            
            System.load(libFile.getAbsolutePath());
            loaded = true;
            LOGGER.info("Loaded native library from custom path: " + libraryPath);
        }
    }
    
    /**
     * Print diagnostic information about the native library loading.
     * Useful for debugging deployment issues.
     */
    public static void printDiagnostics() {
        System.out.println("=== FastCollection Native Library Diagnostics ===");
        System.out.println("Platform: " + PLATFORM);
        System.out.println("OS Name: " + System.getProperty("os.name"));
        System.out.println("OS Arch: " + System.getProperty("os.arch"));
        System.out.println("Java Version: " + System.getProperty("java.version"));
        System.out.println("Expected Library: " + PLATFORM.getLibraryName());
        System.out.println("Legacy Library: " + PLATFORM.getLegacyLibraryName());
        System.out.println("Library Loaded: " + loaded);
        if (extractedLibraryPath != null) {
            System.out.println("Extracted To: " + extractedLibraryPath);
        }
        System.out.println("java.library.path: " + System.getProperty("java.library.path"));
        System.out.println("\nResource paths to search:");
        for (String path : PLATFORM.getAlternativeResourcePaths()) {
            boolean exists = NativeLibraryLoader.class.getResource(path) != null;
            System.out.println("  " + path + " -> " + (exists ? "FOUND" : "not found"));
        }
        System.out.println("================================================");
    }
}
