package com.night.backgroundchange;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.os.Bundle;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import androidx.appcompat.app.AppCompatActivity;

public class MainActivity extends AppCompatActivity {
    private StructuralGameView gameView;

    static {
        System.loadLibrary("native-lib");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        getWindow().getDecorView().setSystemUiVisibility(
                View.SYSTEM_UI_FLAG_FULLSCREEN |
                View.SYSTEM_UI_FLAG_HIDE_NAVIGATION |
                View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY);

        gameView = new StructuralGameView(this);
        setContentView(gameView);
    }

    class StructuralGameView extends SurfaceView implements SurfaceHolder.Callback, Runnable {
        private Thread renderThread;
        private volatile boolean isRunning;
        private final SurfaceHolder surfaceHolder;

        public StructuralGameView(Context context) {
            super(context);
            surfaceHolder = getHolder();
            surfaceHolder.addCallback(this);

            initNativeEngine(true);

            int[] structuralDrawableIds = {
                R.drawable.arrow, R.drawable.tile, R.drawable.glow, R.drawable.back,
                R.drawable.home, R.drawable.retry, R.drawable.next, R.drawable.play,
                R.drawable.paused, R.drawable.settings, R.drawable.sound_on,
                R.drawable.soundoff, R.drawable.tick, R.drawable.star,
                R.drawable.hint, R.drawable.close, R.drawable.lock
            };

            for (int i = 0; i < structuralDrawableIds.length; i++) {
                try {
                    Bitmap bmp = BitmapFactory.decodeResource(getResources(), structuralDrawableIds[i]);
                    if (bmp != null) {
                        nativePushAsset(i, bmp);
                    }
                } catch (Exception e) {
                    android.util.Log.e("UIStructure", "Failed loading index: " + i);
                }
            }
        }

        @Override
        public void surfaceCreated(SurfaceHolder holder) {
            isRunning = true;
            renderThread = new Thread(this);
            renderThread.start();
        }

        @Override
        public void run() {
            while (isRunning) {
                if (!surfaceHolder.getSurface().isValid()) continue;
                Canvas canvas = surfaceHolder.lockCanvas();
                if (canvas != null) {
                    nativeRender(canvas);
                    surfaceHolder.unlockCanvasAndPost(canvas);
                }
                try {
                    Thread.sleep(16);
                } catch (InterruptedException e) {
                    Thread.currentThread().interrupt();
                }
            }
        }

        @Override
        public boolean onTouchEvent(MotionEvent event) {
            if (event.getAction() == MotionEvent.ACTION_DOWN) {
                nativeOnTouch(event.getX(), event.getY());
            }
            return true;
        }

        @Override
        public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
            nativeOnResize(width, height);
        }

        @Override
        public void surfaceDestroyed(SurfaceHolder holder) {
            isRunning = false;
            try {
                if (renderThread != null) renderThread.join();
            } catch (InterruptedException e) {
                Thread.currentThread().interrupt();
            }
        }
    }

    public native void initNativeEngine(boolean darkTheme);
    public native void nativePushAsset(int index, Bitmap bitmap);
    public native void nativeRender(Canvas canvas);
    public native void nativeOnTouch(float x, float y);
    public native void nativeOnResize(int w, int h);
}
