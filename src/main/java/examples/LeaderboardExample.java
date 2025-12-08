/**
 * FastCollection v1.0.0 - Leaderboard Example
 * 
 * Implements a game leaderboard with rankings using FastCollectionMap.
 * 
 * Copyright © 2025-2030, Ashutosh Sinha (ajsinha@gmail.com)
 * Patent Pending
 */
package examples;

import com.kuber.fastcollection.*;
import java.io.Serializable;
import java.util.*;
import java.util.stream.Collectors;

public class LeaderboardExample {
    
    public static class PlayerScore implements Serializable, Comparable<PlayerScore> {
        private static final long serialVersionUID = 1L;
        
        public String playerId;
        public String playerName;
        public long score;
        public int gamesPlayed;
        public long highestScore;
        public long lastUpdated;
        
        public PlayerScore(String playerId, String playerName) {
            this.playerId = playerId;
            this.playerName = playerName;
            this.score = 0;
            this.gamesPlayed = 0;
            this.highestScore = 0;
            this.lastUpdated = System.currentTimeMillis();
        }
        
        public void addScore(long points) {
            this.score += points;
            this.gamesPlayed++;
            this.highestScore = Math.max(this.highestScore, points);
            this.lastUpdated = System.currentTimeMillis();
        }
        
        public double getAverageScore() {
            return gamesPlayed > 0 ? (double) score / gamesPlayed : 0;
        }
        
        @Override
        public int compareTo(PlayerScore other) {
            return Long.compare(other.score, this.score);  // Descending
        }
        
        @Override
        public String toString() {
            return String.format("%s: %,d pts (%d games, avg: %.1f, best: %,d)",
                playerName, score, gamesPlayed, getAverageScore(), highestScore);
        }
    }
    
    public static class Leaderboard {
        private final FastCollectionMap<String, PlayerScore> scores;
        private final String name;
        
        public Leaderboard(String name, String path) {
            this.name = name;
            this.scores = new FastCollectionMap<>(path, 32 * 1024 * 1024, true);
        }
        
        public void recordScore(String playerId, String playerName, long points) {
            PlayerScore player = scores.get(playerId);
            
            if (player == null) {
                player = new PlayerScore(playerId, playerName);
            }
            
            player.addScore(points);
            scores.put(playerId, player);
            
            System.out.printf("  %s scored %,d points (total: %,d)\n",
                playerName, points, player.score);
        }
        
        public PlayerScore getPlayer(String playerId) {
            return scores.get(playerId);
        }
        
        public List<PlayerScore> getTopPlayers(int count) {
            List<PlayerScore> allPlayers = new ArrayList<>();
            
            // Collect all players
            // In a real implementation, you'd have an iterator
            // For now, we'll track player IDs separately
            
            return allPlayers.stream()
                .sorted()
                .limit(count)
                .collect(Collectors.toList());
        }
        
        public List<PlayerScore> getAllPlayersSorted() {
            // This is a simplified implementation
            // In production, you'd maintain a sorted index
            List<PlayerScore> players = new ArrayList<>();
            
            // Iterate through known players
            for (String playerId : getKnownPlayerIds()) {
                PlayerScore player = scores.get(playerId);
                if (player != null) {
                    players.add(player);
                }
            }
            
            Collections.sort(players);
            return players;
        }
        
        // In real implementation, maintain player ID index
        private Set<String> knownPlayerIds = new HashSet<>();
        
        public Set<String> getKnownPlayerIds() {
            return knownPlayerIds;
        }
        
        public void trackPlayer(String playerId) {
            knownPlayerIds.add(playerId);
        }
        
        public int getPlayerCount() {
            return knownPlayerIds.size();
        }
        
        public void close() {
            scores.close();
        }
    }
    
    public static void main(String[] args) {
        System.out.println("FastCollection v1.0.0 - Leaderboard Example");
        System.out.println("===========================================\n");
        
        Leaderboard leaderboard = new Leaderboard("GameLeaderboard", "/tmp/leaderboard.fc");
        
        try {
            // Define players
            String[][] players = {
                {"p1", "Alice"},
                {"p2", "Bob"},
                {"p3", "Charlie"},
                {"p4", "Diana"},
                {"p5", "Eve"}
            };
            
            // Track players
            for (String[] p : players) {
                leaderboard.trackPlayer(p[0]);
            }
            
            // Simulate game sessions
            System.out.println("=== Game Session 1 ===\n");
            leaderboard.recordScore("p1", "Alice", 1500);
            leaderboard.recordScore("p2", "Bob", 2300);
            leaderboard.recordScore("p3", "Charlie", 1800);
            leaderboard.recordScore("p4", "Diana", 2100);
            leaderboard.recordScore("p5", "Eve", 1200);
            
            System.out.println("\n=== Game Session 2 ===\n");
            leaderboard.recordScore("p1", "Alice", 2800);
            leaderboard.recordScore("p2", "Bob", 1900);
            leaderboard.recordScore("p3", "Charlie", 2500);
            leaderboard.recordScore("p5", "Eve", 3100);
            
            System.out.println("\n=== Game Session 3 ===\n");
            leaderboard.recordScore("p1", "Alice", 2200);
            leaderboard.recordScore("p4", "Diana", 2900);
            leaderboard.recordScore("p3", "Charlie", 1600);
            
            // Display leaderboard
            System.out.println("\n╔══════════════════════════════════════════════════════════════╗");
            System.out.println("║                    🏆 LEADERBOARD 🏆                          ║");
            System.out.println("╠══════════════════════════════════════════════════════════════╣");
            
            List<PlayerScore> sorted = leaderboard.getAllPlayersSorted();
            int rank = 1;
            for (PlayerScore player : sorted) {
                String medal = rank == 1 ? "🥇" : rank == 2 ? "🥈" : rank == 3 ? "🥉" : "  ";
                System.out.printf("║ %s #%d %-50s ║\n", medal, rank, player);
                rank++;
            }
            
            System.out.println("╚══════════════════════════════════════════════════════════════╝");
            
            // Individual player stats
            System.out.println("\n=== Individual Stats ===\n");
            
            PlayerScore alice = leaderboard.getPlayer("p1");
            if (alice != null) {
                System.out.println("Alice's Stats:");
                System.out.printf("  Total Score: %,d\n", alice.score);
                System.out.printf("  Games Played: %d\n", alice.gamesPlayed);
                System.out.printf("  Average Score: %.1f\n", alice.getAverageScore());
                System.out.printf("  Highest Score: %,d\n", alice.highestScore);
            }
            
        } finally {
            leaderboard.close();
        }
    }
}
