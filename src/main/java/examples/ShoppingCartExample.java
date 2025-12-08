/**
 * FastCollection v1.0.0 - Shopping Cart Example
 * 
 * Implements a persistent shopping cart with auto-expiry.
 * 
 * Copyright © 2025-2030, Ashutosh Sinha (ajsinha@gmail.com)
 * Patent Pending
 */
package examples;

import com.kuber.fastcollection.*;
import java.io.Serializable;
import java.math.BigDecimal;
import java.math.RoundingMode;
import java.util.*;

public class ShoppingCartExample {
    
    public static class CartItem implements Serializable {
        private static final long serialVersionUID = 1L;
        
        public String productId;
        public String productName;
        public BigDecimal unitPrice;
        public int quantity;
        public long addedAt;
        
        public CartItem(String productId, String productName, BigDecimal unitPrice, int quantity) {
            this.productId = productId;
            this.productName = productName;
            this.unitPrice = unitPrice;
            this.quantity = quantity;
            this.addedAt = System.currentTimeMillis();
        }
        
        public BigDecimal getSubtotal() {
            return unitPrice.multiply(BigDecimal.valueOf(quantity));
        }
        
        @Override
        public String toString() {
            return String.format("%s x%d @ $%.2f = $%.2f",
                productName, quantity, unitPrice, getSubtotal());
        }
    }
    
    public static class ShoppingCart implements Serializable {
        private static final long serialVersionUID = 1L;
        
        public String cartId;
        public String userId;
        public Map<String, CartItem> items;
        public long createdAt;
        public long lastModified;
        public String couponCode;
        public BigDecimal discountPercent;
        
        public ShoppingCart(String cartId, String userId) {
            this.cartId = cartId;
            this.userId = userId;
            this.items = new HashMap<>();
            this.createdAt = System.currentTimeMillis();
            this.lastModified = this.createdAt;
            this.discountPercent = BigDecimal.ZERO;
        }
        
        public void addItem(CartItem item) {
            if (items.containsKey(item.productId)) {
                CartItem existing = items.get(item.productId);
                existing.quantity += item.quantity;
            } else {
                items.put(item.productId, item);
            }
            lastModified = System.currentTimeMillis();
        }
        
        public void removeItem(String productId) {
            items.remove(productId);
            lastModified = System.currentTimeMillis();
        }
        
        public void updateQuantity(String productId, int quantity) {
            CartItem item = items.get(productId);
            if (item != null) {
                if (quantity <= 0) {
                    items.remove(productId);
                } else {
                    item.quantity = quantity;
                }
                lastModified = System.currentTimeMillis();
            }
        }
        
        public void applyCoupon(String code, BigDecimal discountPercent) {
            this.couponCode = code;
            this.discountPercent = discountPercent;
            lastModified = System.currentTimeMillis();
        }
        
        public BigDecimal getSubtotal() {
            return items.values().stream()
                .map(CartItem::getSubtotal)
                .reduce(BigDecimal.ZERO, BigDecimal::add);
        }
        
        public BigDecimal getDiscount() {
            return getSubtotal()
                .multiply(discountPercent)
                .divide(BigDecimal.valueOf(100), 2, RoundingMode.HALF_UP);
        }
        
        public BigDecimal getTotal() {
            return getSubtotal().subtract(getDiscount());
        }
        
        public int getItemCount() {
            return items.values().stream()
                .mapToInt(i -> i.quantity)
                .sum();
        }
    }
    
    public static class CartService {
        private final FastCollectionMap<String, ShoppingCart> carts;
        private final int cartTTL;  // seconds
        
        public CartService(String path, int cartTTLHours) {
            this.carts = new FastCollectionMap<>(path, 64 * 1024 * 1024, true);
            this.cartTTL = cartTTLHours * 3600;
        }
        
        public ShoppingCart getOrCreateCart(String userId) {
            String cartId = "cart_" + userId;
            ShoppingCart cart = carts.get(cartId);
            
            if (cart == null) {
                cart = new ShoppingCart(cartId, userId);
                carts.put(cartId, cart, cartTTL);
            }
            
            return cart;
        }
        
        public void saveCart(ShoppingCart cart) {
            carts.put(cart.cartId, cart, cartTTL);
        }
        
        public void deleteCart(String userId) {
            carts.remove("cart_" + userId);
        }
        
        public long getCartTTL(String userId) {
            return carts.getTTL("cart_" + userId);
        }
        
        public void close() {
            carts.close();
        }
    }
    
    public static void main(String[] args) {
        System.out.println("FastCollection v1.0.0 - Shopping Cart Example");
        System.out.println("=============================================\n");
        
        // Cart expires after 24 hours of inactivity
        CartService cartService = new CartService("/tmp/carts.fc", 24);
        
        try {
            // Simulate user shopping
            String userId = "user_12345";
            
            System.out.println("🛒 Shopping Session for " + userId + "\n");
            
            // Get or create cart
            ShoppingCart cart = cartService.getOrCreateCart(userId);
            
            // Add items
            System.out.println("Adding items to cart...\n");
            
            cart.addItem(new CartItem("SKU001", "Wireless Mouse", new BigDecimal("29.99"), 1));
            cart.addItem(new CartItem("SKU002", "USB-C Cable (3-pack)", new BigDecimal("12.99"), 2));
            cart.addItem(new CartItem("SKU003", "Laptop Stand", new BigDecimal("49.99"), 1));
            cart.addItem(new CartItem("SKU004", "Webcam HD 1080p", new BigDecimal("79.99"), 1));
            
            cartService.saveCart(cart);
            
            // Display cart
            printCart(cart);
            
            // Update quantity
            System.out.println("\nUpdating USB-C Cable quantity to 3...");
            cart.updateQuantity("SKU002", 3);
            cartService.saveCart(cart);
            
            // Apply coupon
            System.out.println("Applying coupon code 'SAVE10' for 10% off...\n");
            cart.applyCoupon("SAVE10", new BigDecimal("10"));
            cartService.saveCart(cart);
            
            // Display updated cart
            printCart(cart);
            
            // Show cart TTL
            long ttl = cartService.getCartTTL(userId);
            System.out.printf("\nCart expires in: %.1f hours\n", ttl / 3600.0);
            
            // Remove an item
            System.out.println("\nRemoving Laptop Stand from cart...");
            cart.removeItem("SKU003");
            cartService.saveCart(cart);
            
            // Final cart
            System.out.println("\n=== Final Cart ===");
            printCart(cart);
            
        } finally {
            cartService.close();
        }
    }
    
    private static void printCart(ShoppingCart cart) {
        System.out.println("┌──────────────────────────────────────────────────────┐");
        System.out.println("│                    SHOPPING CART                     │");
        System.out.println("├──────────────────────────────────────────────────────┤");
        
        for (CartItem item : cart.items.values()) {
            System.out.printf("│ %-40s $%7.2f │\n", 
                item.productName + " x" + item.quantity,
                item.getSubtotal());
        }
        
        System.out.println("├──────────────────────────────────────────────────────┤");
        System.out.printf("│ Subtotal (%d items): %27s │\n", 
            cart.getItemCount(), "$" + cart.getSubtotal());
        
        if (cart.couponCode != null) {
            System.out.printf("│ Discount (%s - %s%%): %23s │\n",
                cart.couponCode, cart.discountPercent, "-$" + cart.getDiscount());
        }
        
        System.out.println("├──────────────────────────────────────────────────────┤");
        System.out.printf("│ TOTAL: %42s │\n", "$" + cart.getTotal());
        System.out.println("└──────────────────────────────────────────────────────┘");
    }
}
