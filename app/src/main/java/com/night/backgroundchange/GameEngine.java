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

    public GameEngine(Context context, MainActivity activity, FrameLayout container) {
        super(context);
        this.activity = activity;
        this.container = container;

        this.surfaceHolder = getHolder();
        this.surfaceHolder.addCallback(this);

        this.paint = new Paint();
        paint.setAntiAlias(true);
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
        // Place your level loading configuration logic here
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
                        // Update game states and handle drawing tasks
                        renderGame(canvas);
                    }
                } catch (Exception e) {
                    e.printStackTrace();
                } finally {
                    surfaceHolder.unlockCanvasAndPost(canvas);
                }
            }

            try {
                Thread.sleep(16); // Lock at 60 Frames Per Second smoothly
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }
    }

    private void renderGame(Canvas canvas) {
        // Clear screen with a dark aesthetic theme color
        canvas.drawColor(Color.parseColor("#121214")); 

        // =================================================================
        // 🛠️ DEVELOPER NOTE: PLACE YOUR GAME DRAWING LOGIC HERE!
        // You can use 'canvas.drawBitmap' or 'canvas.drawRect' to draw
        // your puzzle arrows, background graphics, and level items.
        // =================================================================

        paint.setColor(Color.WHITE);
        paint.setTextSize(55);
        paint.setTextAlign(Paint.Align.CENTER);
        canvas.drawText("Night Shadow Game Activated", canvas.getWidth() / 2, canvas.getHeight() / 2, paint);
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        if (event.getAction() == MotionEvent.ACTION_DOWN) {
            float touchX = event.getX();
            float touchY = event.getY();

            // Handle arrow tap mechanics or JNI boundary check moves here
            if (activity != null) {
                activity.playSound(false); // Play click audio stream tracking click behavior
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
