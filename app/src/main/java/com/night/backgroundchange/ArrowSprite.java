package com.night.backgroundchange;

import android.graphics.Rect;

public class ArrowSprite {
    public int id, dir;
    public float currentX, currentY, scale = 1.0f;
    private boolean flying = false;
    private float speed = 40f;

    public ArrowSprite(int id, int gridX, int gridY, int dir) {
        this.id = id;
        this.dir = dir;
        this.currentX = 150 + gridX * 200; // Offset + Tile Size
        this.currentY = 300 + gridY * 200;
    }

    public void update() {
        if (flying) {
            if (dir == 0) currentY -= speed;
            else if (dir == 1) currentX += speed;
            else if (dir == 2) currentY += speed;
            else if (dir == 3) currentX -= speed;
            scale *= 0.98f;
        }
    }

    public void startFlying() { flying = true; }
    public void shake() { /* Visual feedback for blocked */ }
    public boolean isOffScreen() { return currentX < -200 || currentX > 2000 || currentY < -200 || currentY > 3000; }
    public Rect getBounds() { return new Rect((int)currentX, (int)currentY, (int)currentX+180, (int)currentY+180); }
}
