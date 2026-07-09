package com.night.backgroundchange;

import java.util.Random;

public class Particle {
    public float x, y, vx, vy, alpha = 1.0f;
    private Random res = new Random();

    public Particle(float x, float y) {
        this.x = x; this.y = y;
        this.vx = (res.nextFloat() - 0.5f) * 10f;
        this.vy = (res.nextFloat() - 0.5f) * 10f;
    }

    public boolean update() {
        x += vx; y += vy;
        alpha -= 0.05f;
        return alpha <= 0;
    }
}
