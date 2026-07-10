package com.night.backgroundchange;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
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

    // Game Matrix Metrics
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
            // Populate multiple blocks to build a real map layout
            blocks.add(new Block(1, 1, false));
            blocks.add(new Block(1, 4, false));
            blocks.add(new Block(3, 2, false));
            blocks.add(new Block(5, 3, false));
            
            // Add Target destination flags (Red Blocks)
            blocks.add(new Block(7, 2, true));
            blocks.add(new Block(7, 4, true));

            // Set up our interactive arrows scattered on the grid fields
            arrows.add(new Arrow(2, 5, "UP", 1));
            arrows.add(new Arrow(4, 6, "LEFT", 2));
            arrows.add(new Arrow(1, 3, "DOWN", 3));
            arrows.add(new Arrow(3, 1, "RIGHT", 4));

            levelGrid[1][1] = 1; levelGrid[1][4] = 1; levelGrid[3][2] = 1; levelGrid[5][3] = 1;
            levelGrid[7][2] = 3; levelGrid[7][4] = 3; 
        } else {
            blocks.add(new Block(2, 2, false));
            blocks.add(new Block(4, 4, true));
            arrows.add(new Arrow(1, 4, "RIGHT", 5));
            levelGrid[2][2] = 1; levelGrid[4][4] = 3;
        }

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
                // Safeguard JNI binding
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
                }
                // FIXED: Removed the unnecessary finally block completely to avoid symbol errors
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
                
                if (arrow.currentX < 0 || arrow.currentX >= cols || arrow.currentY < 0 || arrow.currentY >= rows) {
                    arrow.isMoving = false;
                    arrow.currentX = Math.max(0, Math.min(arrow.currentX, cols - 1));
                    arrow.currentY = Math.max(0, Math.min(arrow.currentY, rows - 1));
                }
            }
        }
    }

    private void renderGame(Canvas canvas) {
        if (bgBitmap != null) {
            canvas.drawBitmap(bgBitmap, 0, 0, null);
        } else {
            canvas.drawColor(Color.parseColor("#0F1322")); 
        }

        cellSize = Math.min(canvas.getWidth() / cols, canvas.getHeight() / rows);
        offsetX = (canvas.getWidth() - (cols * cellSize)) / 2;
        offsetY = (canvas.getHeight() - (rows * cellSize)) / 2;

        paint.setStyle(Paint.Style.STROKE);
        paint.setColor(Color.parseColor("#1D2640"));
        paint.setStrokeWidth(3);
        for (int r = 0; r <= rows; r++) {
            canvas.drawLine(offsetX, offsetY + r * cellSize, offsetX + cols * cellSize, offsetY + r * cellSize, paint);
        }
        for (int c = 0; c <= cols; c++) {
            canvas.drawLine(offsetX + c * cellSize, offsetY, offsetX + c * cellSize, offsetY + rows * cellSize, paint);
        }

        paint.setStyle(Paint.Style.FILL);
        for (Block block : blocks) {
            Bitmap b = block.isTarget ? blockTarget : blockNormal;
            int left = offsetX + block.col * cellSize;
            int top = offsetY + block.row * cellSize;
            
            if (b != null) {
                canvas.drawBitmap(b, left, top, null);
            } else {
                paint.setColor(block.isTarget ? Color.parseColor("#EF476F") : Color.parseColor("#FFD166"));
                canvas.drawRoundRect(left + 8, top + 8, left + cellSize - 8, top + cellSize - 8, 16, 16, paint);
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

            int left = offsetX + arrow.currentX * cellSize;
            int top = offsetY + arrow.currentY * cellSize;

            if (aBitmap != null) {
                canvas.drawBitmap(aBitmap, left, top, null);
            } else {
                paint.setColor(Color.parseColor("#06D6A0"));
                drawVectorArrow(canvas, left, top, cellSize, arrow.direction);
            }
        }
    }

    private void drawVectorArrow(Canvas canvas, int left, int top, int size, String direction) {
        Path path = new Path();
        float padding = size * 0.25f;
        float centerX = left + size / 2f;
        float centerY = top + size / 2f;

        if ("UP".equals(direction)) {
            path.moveTo(centerX, top + padding);
            path.lineTo(left + padding, top + size - padding);
            path.lineTo(left + size - padding, top + size - padding);
        } else if ("DOWN".equals(direction)) {
            path.moveTo(centerX, top + size - padding);
            path.lineTo(left + padding, top + padding);
            path.lineTo(left + size - padding, top + padding);
        } else if ("LEFT".equals(direction)) {
            path.moveTo(left + padding, centerY);
            path.lineTo(left + size - padding, top + padding);
            path.lineTo(left + size - padding, top + size - padding);
        } else if ("RIGHT".equals(direction)) {
            path.moveTo(left + size - padding, centerY);
            path.lineTo(left + padding, top + padding);
            path.lineTo(left + padding, top + size - padding);
        }
        path.close();
        canvas.drawPath(path, paint);
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
                        // Safe JNI fallback
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

    // Subclasses
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
