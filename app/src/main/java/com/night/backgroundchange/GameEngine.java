package com.night.backgroundchange;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Path;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.widget.FrameLayout;
import java.util.ArrayList;
import java.util.List;

public class GameEngine extends SurfaceView implements SurfaceHolder.Callback, Runnable {

    private MainActivity activity;
    private FrameLayout container; 
    private Thread gameThread;
    private boolean isRunning = false;
    private SurfaceHolder surfaceHolder;
    private Paint paint;

    // Grid System
    private int rows = 9;
    private int cols = 6;
    private int cellSize;
    private int offsetX;
    private int offsetY;

    // Game Elements
    private int currentLevel = 1;
    private List<Arrow> arrows;
    private List<TargetZone> targets;
    private boolean isSimulationRunning = false;
    private float simulationSpeed = 0.15f; // Control smooth transit movement

    public GameEngine(Context context, MainActivity activity, FrameLayout container) {
        super(context);
        this.activity = activity;
        this.container = container; 

        this.surfaceHolder = getHolder();
        this.surfaceHolder.addCallback(this);

        this.paint = new Paint();
        this.paint.setAntiAlias(true);
        setFocusable(true);

        this.arrows = new ArrayList<>();
        this.targets = new ArrayList<>();

        loadLevel(currentLevel);
    }

    public void loadLevel(int level) {
        this.currentLevel = level;
        this.arrows.clear();
        this.targets.clear();
        this.isSimulationRunning = false;

        if (level == 1) {
            // Set up Destination target nodes (Goal spots)
            targets.add(new TargetZone(2, 1, "#EF476F")); // Red Target
            targets.add(new TargetZone(4, 4, "#FFD166")); // Yellow Target

            // Place interactive puzzle arrows that users rotate to form paths
            arrows.add(new Arrow(1, 4, "RIGHT", 1));
            arrows.add(new Arrow(2, 4, "UP", 2));
            arrows.add(new Arrow(4, 2, "LEFT", 3));
        } else {
            targets.add(new TargetZone(3, 3, "#06D6A0"));
            arrows.add(new Arrow(1, 1, "RIGHT", 1));
        }
    }

    public void startPathSimulation() {
        this.isSimulationRunning = true;
    }

    public void resume() {
        isRunning = true;
        gameThread = new Thread(this);
        gameThread.start();
    }

    public void pause() {
        isRunning = false;
        try {
            if (gameThread != null) {
                gameThread.join();
            }
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
    }

    @Override
    public void run() {
        while (isRunning) {
            if (!surfaceHolder.getSurface().isValid()) {
                continue;
            }

            Canvas canvas = surfaceHolder.lockCanvas();
            if (canvas != null) {
                try {
                    synchronized (surfaceHolder) {
                        updateGameLogic();
                        renderGame(canvas);
                    }
                } catch (Exception e) {
                    e.printStackTrace();
                }
                surfaceHolder.unlockCanvasAndPost(canvas);
            }

            try {
                Thread.sleep(16); 
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }
    }

    private void updateGameLogic() {
        if (!isSimulationRunning) return;

        boolean anyArrowMoving = false;
        for (Arrow arrow : arrows) {
            if (arrow.isMoving) {
                anyArrowMoving = true;
                arrow.animateProgress += simulationSpeed;

                if (arrow.animateProgress >= 1.0f) {
                    arrow.animateProgress = 0.0f;
                    
                    // Advance coordinates based on direction vectors
                    switch (arrow.direction) {
                        case "UP": arrow.gridY--; break;
                        case "DOWN": arrow.gridY++; break;
                        case "LEFT": arrow.gridX--; break;
                        case "RIGHT": arrow.gridX++; break;
                    }

                    // Check boundaries or goal intersections
                    if (arrow.gridX < 0 || arrow.gridX >= cols || arrow.gridY < 0 || arrow.gridY >= rows) {
                        arrow.isMoving = false;
                    }
                    
                    checkTargetCollisions(arrow);
                }
            }
        }
        
        // Auto start movement on simulation trigger
        if (isSimulationRunning && !anyArrowMoving) {
            for (Arrow arrow : arrows) {
                if (arrow.gridX >= 0 && arrow.gridX < cols && arrow.gridY >= 0 && arrow.gridY < rows) {
                    arrow.isMoving = true;
                }
            }
        }
    }

    private void checkTargetCollisions(Arrow arrow) {
        for (TargetZone target : targets) {
            if (target.gridX == arrow.gridX && target.gridY == arrow.gridY) {
                target.isCleared = true;
                arrow.isMoving = false;
                // Safe JNI audio dispatch hook
                try { if (activity != null) activity.playSound(true); } catch (Throwable ignored) {}
            }
        }
    }

    private void renderGame(Canvas canvas) {
        // High-fidelity Deep Cosmic Blueprint Background
        canvas.drawColor(Color.parseColor("#0A0E1A")); 

        cellSize = Math.min(canvas.getWidth() / cols, canvas.getHeight() / rows);
        offsetX = (canvas.getWidth() - (cols * cellSize)) / 2;
        offsetY = (canvas.getHeight() - (rows * cellSize)) / 2;

        // Draw Modern Grid Array Lines
        paint.setStyle(Paint.Style.STROKE);
        paint.setStrokeWidth(2);
        for (int r = 0; r <= rows; r++) {
            paint.setColor(Color.parseColor(r % 3 == 0 ? "#1E2942" : "#131A2E"));
            canvas.drawLine(offsetX, offsetY + r * cellSize, offsetX + cols * cellSize, offsetY + r * cellSize, paint);
        }
        for (int c = 0; c <= cols; c++) {
            paint.setColor(Color.parseColor(c % 3 == 0 ? "#1E2942" : "#131A2E"));
            canvas.drawLine(offsetX + c * cellSize, offsetY, offsetX + c * cellSize, offsetY + rows * cellSize, paint);
        }

        // Render Targets (Goal Nodes)
        paint.setStyle(Paint.Style.FILL);
        for (TargetZone target : targets) {
            int cx = offsetX + target.gridX * cellSize + cellSize / 2;
            int cy = offsetY + target.gridY * cellSize + cellSize / 2;
            
            if (target.isCleared) {
                paint.setColor(Color.parseColor("#06D6A0")); // Pulse green on solved
                canvas.drawCircle(cx, cy, cellSize * 0.4f, paint);
            } else {
                paint.setColor(Color.parseColor(target.colorHex));
                canvas.drawCircle(cx, cy, cellSize * 0.35f, paint);
                paint.setStyle(Paint.Style.STROKE);
                paint.setColor(Color.WHITE);
                paint.setStrokeWidth(4);
                canvas.drawCircle(cx, cy, cellSize * 0.2f, paint);
                paint.setStyle(Paint.Style.FILL);
            }
        }

        // Render Elegant Glowing Vector Arrows
        for (Arrow arrow : arrows) {
            float currentRenderX = arrow.gridX;
            float currentRenderY = arrow.gridY;

            // Interpolate movement smoothly across grid rows during running phases
            if (arrow.isMoving) {
                switch (arrow.direction) {
                    case "UP": currentRenderY -= arrow.animateProgress; break;
                    case "DOWN": currentRenderY += arrow.animateProgress; break;
                    case "LEFT": currentRenderX -= arrow.animateProgress; break;
                    case "RIGHT": currentRenderX += arrow.animateProgress; break;
                }
            }

            float left = offsetX + currentRenderX * cellSize;
            float top = offsetY + currentRenderY * cellSize;

            paint.setColor(Color.parseColor("#3A86FF")); // Premium Cyber Blue Interactive Accent
            drawProceduralArrow(canvas, left, top, cellSize, arrow.direction);
        }
    }

    private void drawProceduralArrow(Canvas canvas, float left, float top, float size, String direction) {
        Path path = new Path();
        float padding = size * 0.2f;
        float centerX = left + size / 2f;
        float centerY = top + size / 2f;

        if ("UP".equals(direction)) {
            path.moveTo(centerX, top + padding);
            path.lineTo(left + padding, top + size - padding);
            path.lineTo(centerX, top + size - padding * 1.5f);
            path.lineTo(left + size - padding, top + size - padding);
        } else if ("DOWN".equals(direction)) {
            path.moveTo(centerX, top + size - padding);
            path.lineTo(left + padding, top + padding);
            path.lineTo(centerX, top + padding * 1.5f);
            path.lineTo(left + size - padding, top + padding);
        } else if ("LEFT".equals(direction)) {
            path.moveTo(left + padding, centerY);
            path.lineTo(left + size - padding, top + padding);
            path.lineTo(left + size - padding * 1.5f, centerY);
            path.lineTo(left + size - padding, top + size - padding);
        } else if ("RIGHT".equals(direction)) {
            path.moveTo(left + size - padding, centerY);
            path.lineTo(left + padding, top + padding);
            path.lineTo(left + padding * 1.5f, centerY);
            path.lineTo(left + padding, top + size - padding);
        }
        path.close();
        
        paint.setStyle(Paint.Style.FILL);
        canvas.drawPath(path, paint);
        
        // Add a clean high-contrast outer ring highlight
        paint.setStyle(Paint.Style.STROKE);
        paint.setColor(Color.parseColor("#4CC9F0"));
        paint.setStrokeWidth(3);
        canvas.drawRoundRect(left + 4, top + 4, left + size - 4, top + size - 4, 12, 12, paint);
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        if (event.getAction() == MotionEvent.ACTION_DOWN && !isSimulationRunning) {
            float tx = event.getX();
            float ty = event.getY();

            int clickedCol = (int) ((tx - offsetX) / cellSize);
            int clickedRow = (int) ((ty - offsetY) / cellSize);

            for (Arrow arrow : arrows) {
                if (arrow.gridX == clickedCol && arrow.gridY == clickedRow) {
                    // Click cycles directional rotation 90 degrees clockwise
                    arrow.rotateClockwise();
                    
                    try {
                        if (activity != null) activity.playSound(false); 
                    } catch (Throwable ignored) {}
                    
                    // Simple trigger mechanism: double-clicking an arrow launches the execution simulation
                    if (event.getEventTime() - event.getDownTime() > 500) {
                        startPathSimulation();
                    }
                    break;
                }
            }
            performClick();
            return true;
        }
        return super.onTouchEvent(event);
    }

    @Override
    public boolean performClick() {
        return super.performClick();
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {}
    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {}
    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {}

    // Subclasses 
    public static class Arrow {
        public int id;
        public int gridX, gridY;
        public String direction;
        public boolean isMoving = false;
        public float animateProgress = 0.0f;

        public Arrow(int x, int y, String dir, int id) {
            this.gridX = x;
            this.gridY = y;
            this.direction = dir;
            this.id = id;
        }

        public void rotateClockwise() {
            switch (direction) {
                case "UP": direction = "RIGHT"; break;
                case "RIGHT": direction = "DOWN"; break;
                case "DOWN": direction = "LEFT"; break;
                case "LEFT": direction = "UP"; break;
            }
        }
    }

    public static class TargetZone {
        public int gridX, gridY;
        public String colorHex;
        public boolean isCleared = false;

        public TargetZone(int x, int y, String color) {
            this.gridX = x;
            this.gridY = y;
            this.colorHex = color;
        }
    }
}
