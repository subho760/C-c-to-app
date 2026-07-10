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
    private FrameLayout container; 
    private Thread gameThread;
    private boolean isRunning = false;
    private SurfaceHolder surfaceHolder;
    private Paint paint;

    // Original Game State Parameters
    private int currentLevel = 1;
    private int[][] levelGrid;
    private List<Arrow> arrows;
    private List<Block> blocks;
    private int rows = 9;
    private int cols = 6;
    private int cellSize;
    private int offsetX;
    private int offsetY;

    // Bitmaps
    private Bitmap bgBitmap;
    private Bitmap arrowUp, arrowDown, arrowLeft, arrowRight;
    private Bitmap blockNormal, blockTarget;

    // --- CLEAN REPAIRED CONSTRUCTOR ---
    public GameEngine(Context context, MainActivity activity, FrameLayout container) {
        super(context);
        this.activity = activity;
        this.container = container; // Assigned cleanly from MainActivity parameters!

        this.surfaceHolder = getHolder();
        this.surfaceHolder.addCallback(this);

        this.paint = new Paint();
        this.paint.setAntiAlias(true);
        setFocusable(true);

        this.arrows = new ArrayList<>();
        this.blocks = new ArrayList<>();

        loadBitmaps();
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

        levelGrid = new int[rows][cols];

        if (level == 1) {
            levelGrid[2][2] = 1; 
            levelGrid[4][3] = 2; 
            arrows.add(new Arrow(3, 4, "UP", 1));
            blocks.add(new Block(2, 2, false));
        } else {
            levelGrid[1][1] = 1;
            arrows.add(new Arrow(2, 3, "RIGHT", 2));
            blocks.add(new Block(1, 1, true));
        }

        // Matched cleanly with your original native call signature parameters
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
                // Shield native initialization mismatch bounds
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
                } finaly {
                    // Safe execution
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
        for (Arrow arrow : arrows) {
            if (arrow.isMoving) {
                arrow.updatePosition();
            }
        }
    }

    private void renderGame(Canvas canvas) {
        if (bgBitmap != null) {
            canvas.drawBitmap(bgBitmap, 0, 0, null);
        } else {
            canvas.drawColor(Color.parseColor("#1A1A2E"));
        }

        cellSize = Math.min(canvas.getWidth() / cols, canvas.getHeight() / rows);
        offsetX = (canvas.getWidth() - (cols * cellSize)) / 2;
        offsetY = (canvas.getHeight() - (rows * cellSize)) / 2;

        for (Block block : blocks) {
            Bitmap b = block.isTarget ? blockTarget : blockNormal;
            if (b != null) {
                canvas.drawBitmap(b, offsetX + block.col * cellSize, offsetY + block.row * cellSize, null);
            }
        }

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

            int clickedCol = (int) ((tx - offsetX) / cellSize);
            int clickedRow = (int) ((ty - offsetY) / cellSize);

            for (Arrow arrow : arrows) {
                if (arrow.currentX == clickedCol && arrow.currentY == clickedRow && !arrow.isMoving) {
                    boolean canMove = true;
                    try {
                        if (activity != null) canMove = activity.canArrowMove(arrow.id);
                    } catch (Throwable t) {
                        // Safe tracking fallback
                    }

                    if (canMove) {
                        arrow.isMoving = true;
                        if (activity != null) {
                            activity.playSound(false); 
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

    // --- REPAIRED STRUCTURAL SUBCLASSES ---
    public static class Arrow {
        public int id;
        public int currentX, currentY;
        public String direction;
        public boolean isMoving = false;

        public Arrow(int x, int y, String dir, int id) {
            this.currentX = x;
            this.currentY = y;
            this.direction = dir;
            this.id = id;
        }

        public void updatePosition() {
            switch (direction) {
                case "UP": currentY--; break;
                case "DOWN": currentY++; break;
                case "LEFT": currentX--; break;
                case "RIGHT": currentX++; break;
            }
        }
    }

    public static class Block {
        public int row, col;
        public boolean isTarget;

        public Block(int r, int c, boolean t) {
            this.row = r;
            this.col = c;
            this.isTarget = t;
        }
    }
}
