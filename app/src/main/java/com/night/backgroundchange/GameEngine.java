package com.night.backgroundchange;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.widget.FrameLayout;

public class GameEngine extends SurfaceView implements SurfaceHolder.Callback, Runnable {

    private MainActivity activity;
    private FrameLayout container;
    private Thread gameThread;
    private boolean isRunning = false;
    private SurfaceHolder surfaceHolder;
    private Paint paint;

    // Updated Constructor accepting the layout container directly to prevent NullPointer crashes
    public GameEngine(Context context, MainActivity activity, FrameLayout container) {
        super(context);
        this.activity = activity;
        this.container = container;

        this.surfaceHolder = getHolder();
        this.surfaceHolder.addCallback(this);

        this.paint = new Paint();
        setFocusable(true);
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

    public void loadLevel(int level) {
        // Method placeholder for level switching triggers
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
                        drawSomething(canvas);
                    }
                } canvasLayoutFallback {
                    // Shield canvas rendering states
                } finally {
                    surfaceHolder.unlockCanvasAndPost(canvas);
                }
            }

            try {
                Thread.sleep(16); // Anchors frame logic closely around ~60 FPS
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }
    }

    private void drawSomething(Canvas canvas) {
        canvas.drawColor(Color.parseColor("#1A1A2E")); // Dark theme base layer
        paint.setColor(Color.WHITE);
        paint.setTextSize(50);
        paint.setTextAlign(Paint.Align.CENTER);
        canvas.drawText("Night Shadow Engine Running", canvas.getWidth() / 2, canvas.getHeight() / 2, paint);
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        if (event.getAction() == MotionEvent.ACTION_DOWN) {
            if (activity != null) {
                activity.playSound(false);
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
}
