package com.night.backgroundchange;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.widget.FrameLayout;
import java.util.ArrayList;
import java.util.List;

public class GameEngine extends SurfaceView implements SurfaceHolder.Callback, Runnable {

    private MainActivity activity;
    private FrameLayout container; // Safely assigned via constructor parameter
    private Thread gameThread;
    private boolean isRunning = false;
    private SurfaceHolder surfaceHolder;
    private Paint paint;

    // Game states, grids, metrics, and asset variables
    private int currentLevel = 1;
    private int[][] levelGrid;
    private List<Arrow> arrows;
    private List<Block> blocks;
    private int rows = 9;
    private int cols = 6;
    private int cellSize;
    private int offsetX;
    private int offsetY;

    // Bitmap Asset References
    private Bitmap bgBitmap;
    private Bitmap arrowUp, arrowDown, arrowLeft, arrowRight;
    private Bitmap blockNormal, blockTarget;

    // --- SOLVED CONSTRUCTOR ---
    // Instead of looking for a layout ID from XML, it receives the direct reference safely
    public GameEngine(Context context, MainActivity activity, FrameLayout container) {
        super(context);
        this.activity = activity;
        this.container = container;

        this.surfaceHolder = getHolder();
        this.surfaceHolder.addCallback(this);

        this.paint = new Paint();
        this.paint.setAntiAlias(true);
        setFocusable(true);

        // Initialize Lists
        this.arrows = new ArrayList<>();
        this.blocks = new ArrayList<>();

        // Load asset images safely from resources
        loadBitmaps();

        // Initialize the first level structures
        loadLevel(currentLevel);
    }

    private void loadBitmaps() {
        try {
            bgBitmap = BitmapFactory.decodeResource(getResources(), R.drawable.game_bg);
            arrowUp = BitmapFactory.decodeResource(getResources(), R.drawable.arrow_up);
            arrowDown = BitmapFactory.decodeResource(getResources(), R.drawable.arrow_down);
            arrowLeft = BitmapFactory.decodeResource(getResources(), R.drawable.arrow_left);
            arrowRight = BitmapFactory.decodeResource(getResources(), R.drawable.arrow_right);
            blockNormal = BitmapFactory.decodeResource(getResources(), R.drawable.block_normal);
            blockTarget = BitmapFactory.decodeResource(getResources(), R.drawable.block_target);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    public void loadLevel(int level) {
        this.currentLevel = level;
        arrows.clear();
        blocks.clear();

        // Level grid allocation array layout mapping logic
        levelGrid = new int[rows][cols];

        // Hardcoded example fallback grid structure mapping puzzle setups
        if (level == 1) {
            levelGrid[2][2] = 1; // Block
            levelGrid[4][3] = 2; // Arrow setup
            arrows.add(new Arrow(3, 4, "UP", 1));
            blocks.add(new Block(2, 2, false));
        } else {
            levelGrid[1][1] = 1;
            arrows.add(new Arrow(2, 3, "RIGHT", 2));
            blocks.add(new Block(1, 1, true));
        }

        // Notify JNI layer down in C++ framework of data updates
        if (activity != null) {
            int[] linearGrid = new int[rows * cols];
            for (int r = 0; r < rows; r++) {
                for (int c = 0; c < cols; c++) {
                    linearGrid[r * cols + c] = levelGrid[r][c];
                }
            }
            try {
                activity.initNativeLevel(linearGrid);
            } catch (Throwable t) {
                // Handle native library sync boundaries
            }
        }
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
                } finally {
                    surfaceHolder.unlockCanvasAndPost(canvas);
                }
            }

            try {
                Thread.sleep(16); // ~60 FPS Loop
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }
    }

    private void updateGameLogic() {
        // Handle physical movements, collision matrix, and speed transitions
        for (Arrow arrow : arrows) {
            if (arrow.isMoving) {
                arrow.updatePosition();
            }
        }
    }

    private void renderGame(Canvas canvas) {
        // 1. Draw Background asset scaling to frame measurements
        if (bgBitmap != null) {
            canvas.drawBitmap(bgBitmap, 0, 0, null);
        } else {
            canvas.drawColor(Color.parseColor("#1A1A2E"));
        }

        // Calculate dynamic dimensions layout anchors
        cellSize = Math.min(canvas.getWidth() / cols, canvas.getHeight() / rows);
        offsetX = (canvas.getWidth() - (cols * cellSize)) / 2;
        offsetY = (canvas.getHeight() - (rows * cellSize)) / 2;

        // 2. Draw puzzle blocks grids layer
        for (Block block : blocks) {
            Bitmap b = block.isTarget ? blockTarget : blockNormal;
            if (b != null) {
                canvas.drawBitmap(b, offsetX + block.col * cellSize, offsetY + block.row * cellSize, null);
            }
        }

        // 3. Draw active arrow components layers
        for (Arrow arrow : arrows) {
            Bitmap aBitmap = null;
            switch (arrow.direction) {
                case "UP": aBitmap = arrowUp; break;
                case "DOWN": aBitmap = arrowDown; break;
                case "LEFT": aBitmap = arrowLeft; break;
                case "RIGHT": aBitmap = arrowRight; break;
            }
            if (aBitmap != null) {
                canvas.drawBitmap(aBitmap, offsetX + arrow.currentX * cellSize, offsetY + arrow.currentY * cellSize, null);
            }
        }
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        if (event.getAction() == MotionEvent.ACTION_DOWN) {
            float tx = event.getX();
            float ty = event.getY();

            // Calculate grid touch locations back to matrix positions
            int clickedCol = (int) ((tx - offsetX) / cellSize);
            int clickedRow = (int) ((ty - offsetY) / cellSize);

            for (Arrow arrow : arrows) {
                if (arrow.currentX == clickedCol && arrow.currentY == clickedRow && !arrow.isMoving) {
                    // Check logic constraints against native C++ rules before moving
                    boolean canMove = true;
                    try {
                        if (activity != null) canMove = activity.canArrowMove(arrow.id);
                    } catch (Throwable t) {
                        // JNI protection fallback
                    }

                    if (canMove) {
                        arrow.isMoving = true;
                        if (activity != null) {
                            activity.playSound(false); // Action select audio chime
                        }
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

    // --- SUB-CLASS DATA CONTROLLERS ---
    private static class Arrow {
        int id;
        int currentX, currentY;
        String direction;
        boolean isMoving = false;

        Arrow(int x, int y, String dir, int id) {
            this.currentX = x;
            this.currentY = y;
            this.direction = dir;
            this.id = id;
        }

        void updatePosition() {
            switch (direction) {
                case "UP": currentY--; break;
                case "DOWN": currentY++; break;
                case "LEFT": currentX--; break;
                case "RIGHT": currentX++; break;
            }
        }
    }

    private static class Block {
        int row, col;
        boolean isTarget;

        Block(int r, int c, boolean t) {
            this.row = r;
            this.col = c;
            this.isTarget = t;
        }
    }
}
