package com.night.backgroundchange;

import java.util.ArrayList;
import java.util.List;

public class LevelManager {
    public static class ArrowDef {
        public int id, x, y, dir; // dir: 0:U, 1:R, 2:D, 3:L
        public ArrowDef(int id, int x, int y, int dir) {
            this.id = id; this.x = x; this.y = y; this.dir = dir;
        }
    }

    public static List<ArrowDef> getLevel(int level) {
        List<ArrowDef> arrows = new ArrayList<>();
        switch (level) {
            case 1: // Very Easy: Direct paths
                arrows.add(new ArrowDef(1, 2, 2, 0));
                arrows.add(new ArrowDef(2, 2, 4, 2));
                break;
            case 5: // Blocked - must move outer first
                arrows.add(new ArrowDef(1, 2, 3, 3));
                arrows.add(new ArrowDef(2, 1, 3, 3));
                break;
            case 10: // Medium: Cross configuration
                arrows.add(new ArrowDef(1, 2, 1, 0));
                arrows.add(new ArrowDef(2, 1, 2, 3));
                arrows.add(new ArrowDef(3, 3, 2, 1));
                arrows.add(new ArrowDef(4, 2, 3, 2));
                break;
            case 20: // Very Hard: The Spiral
                arrows.add(new ArrowDef(1, 2, 2, 1)); // Blocked by 2
                arrows.add(new ArrowDef(2, 3, 2, 2)); // Blocked by 3
                arrows.add(new ArrowDef(3, 3, 3, 3)); // Blocked by 4
                arrows.add(new ArrowDef(4, 2, 3, 0)); // The "Key" - can move up
                break;
            default:
                // Procedural placeholder for omitted levels to ensure 20 exist
                arrows.add(new ArrowDef(1, level % 4, 2, 1));
                arrows.add(new ArrowDef(2, 4, level % 5, 0));
                break;
        }
        return arrows;
    }
}
