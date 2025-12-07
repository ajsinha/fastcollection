/**
 * FastCollection v1.0.0 - Basic Example
 * 
 * This example demonstrates basic operations with FastCollectionList.
 * 
 * Compile: javac -cp fastcollection-1.0.0.jar BasicExample.java
 * Run:     java -cp .:fastcollection-1.0.0.jar BasicExample
 * 
 * Copyright © 2025-2030, Ashutosh Sinha (ajsinha@gmail.com)
 * Patent Pending
 */
package examples;

import com.kuber.fastcollection.*;

public class BasicExample {
    public static void main(String[] args) {
        System.out.println("FastCollection v1.0.0 - Basic Example");
        System.out.println("=====================================\n");
        
        // Create a persistent list
        String path = "/tmp/basic_example.fc";
        
        try (FastCollectionList<String> list = 
                new FastCollectionList<>(path, String.class, 16 * 1024 * 1024, true)) {
            
            // Add elements
            System.out.println("Adding elements...");
            list.add("Hello");
            list.add("World");
            list.add("FastCollection");
            list.add("is");
            list.add("awesome!");
            
            // Display size
            System.out.println("List size: " + list.size());
            
            // Access elements
            System.out.println("\nElements:");
            for (int i = 0; i < list.size(); i++) {
                System.out.println("  [" + i + "]: " + list.get(i));
            }
            
            // Check contains
            System.out.println("\nContains 'World': " + list.contains("World"));
            System.out.println("Contains 'Java': " + list.contains("Java"));
            
            // Find index
            System.out.println("Index of 'FastCollection': " + list.indexOf("FastCollection"));
            
            // Remove element
            System.out.println("\nRemoving element at index 1...");
            String removed = list.remove(1);
            System.out.println("Removed: " + removed);
            
            // Display updated list
            System.out.println("\nUpdated list:");
            for (int i = 0; i < list.size(); i++) {
                System.out.println("  [" + i + "]: " + list.get(i));
            }
            
            // Clear list
            System.out.println("\nClearing list...");
            list.clear();
            System.out.println("List empty: " + list.isEmpty());
            
            System.out.println("\nExample completed successfully!");
            
        } catch (Exception e) {
            System.err.println("Error: " + e.getMessage());
            e.printStackTrace();
        }
    }
}
