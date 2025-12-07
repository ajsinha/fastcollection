#!/usr/bin/env python3
"""
FastCollection v1.0.0 - Basic Example (Python)

This example demonstrates basic operations with FastList.

Usage: python basic_example.py

Copyright © 2025-2030, Ashutosh Sinha (ajsinha@gmail.com)
Patent Pending
"""

from fastcollection import FastList

def main():
    print("FastCollection v1.0.0 - Basic Example (Python)")
    print("=" * 50)
    print()
    
    path = "/tmp/basic_example_py.fc"
    
    # Use context manager for automatic cleanup
    with FastList(path) as lst:
        # Add elements
        print("Adding elements...")
        items = ["Hello", "World", "FastCollection", "is", "awesome!"]
        for item in items:
            lst.add(item.encode())
        
        # Display size
        print(f"List size: {lst.size()}")
        
        # Access elements
        print("\nElements:")
        for i in range(lst.size()):
            data = lst.get(i)
            if data:
                print(f"  [{i}]: {data.decode()}")
        
        # Check contains
        print(f"\nContains 'World': {lst.contains(b'World')}")
        print(f"Contains 'Java': {lst.contains(b'Java')}")
        
        # Find index
        print(f"Index of 'FastCollection': {lst.index_of(b'FastCollection')}")
        
        # Remove element
        print("\nRemoving element at index 1...")
        removed = lst.remove(1)
        if removed:
            print(f"Removed: {removed.decode()}")
        
        # Display updated list
        print("\nUpdated list:")
        for i in range(lst.size()):
            data = lst.get(i)
            if data:
                print(f"  [{i}]: {data.decode()}")
        
        # Clear list
        print("\nClearing list...")
        lst.clear()
        print(f"List empty: {lst.is_empty()}")
        
        print("\nExample completed successfully!")


if __name__ == "__main__":
    main()
