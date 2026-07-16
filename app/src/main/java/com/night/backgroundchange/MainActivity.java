package com.night.backgroundchange;

import android.content.Context;
import android.content.Intent;
import android.content.res.Configuration;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.ColorMatrixColorFilter;
import android.graphics.Paint;
import android.net.Uri;
import android.os.Bundle;
import android.view.GestureDetector;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.widget.Toast;
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

    public Paint getTintedPaint(int colorHex) {
        Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG);
        int r = (colorHex >> 16) & 0xFF;
        int g = (colorHex >> 8) & 0xFF;
        int b = colorHex & 0xFF;

        float[] matrix = new float[]{
                0, 0, 0, 0, r,
                0, 0, 0, 0, g,
                0, 0, 0, 0, b,
                0, 0, 0, 1, 0
        };
        paint.setColorFilter(new ColorMatrixColorFilter(matrix));
        return paint;
    }

    public void triggerThemeRebuild() {
        runOnUiThread(() -> {
            boolean isSystemDark = (getResources().getConfiguration().uiMode & Configuration.UI_MODE_NIGHT_MASK) 
                    == Configuration.UI_MODE_NIGHT_YES;
            initNativeEngine(isSystemDark);
        });
    }

    public void dispatchPrivacyPolicyIntent() {
        try {
            startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse("https://www.google.com")));
        } catch (Exception e) {
            android.util.Log.e("Launcher", "Privacy redirection error");
        }
    }

    public void dispatchShareIntent() {
        try {
            Intent intent = new Intent(Intent.ACTION_SEND);
            intent.setType("text/plain");
            intent.putExtra(Intent.EXTRA_TEXT, "Play Run Arrow Game!");
            startActivity(Intent.createChooser(intent, "Share Using"));
        } catch (Exception e) {
            android.util.Log.e("Launcher", "Share payload broadcast error");
        }
    }

    public void dispatchRemoveAdsPurchase() {
        runOnUiThread(() -> {
            Toast.makeText(this, "Connecting to Billing Sandbox...", Toast.LENGTH_SHORT).show();
        });
    }

    class StructuralGameView extends SurfaceView implements SurfaceHolder.Callback, Runnable {
        private Thread renderThread;
        private volatile boolean isRunning;
        private final SurfaceHolder surfaceHolder;
        private final GestureDetector gestureDetector;

        public StructuralGameView(Context context) {
            super(context);
            surfaceHolder = getHolder();
            surfaceHolder.addCallback(this);

            gestureDetector = new GestureDetector(context, new GestureDetector.SimpleOnGestureListener() {
                @Override
                public boolean onScroll(MotionEvent e1, MotionEvent e2, float distanceX, float distanceY) {
                    nativeApplyScroll(-distanceY);
                    return true;
                }
            });

            boolean isSystemDark = (getResources().getConfiguration().uiMode & Configuration.UI_MODE_NIGHT_MASK) 
                    == Configuration.UI_MODE_NIGHT_YES;
            initNativeEngine(isSystemDark);

            // Clean, uniform continuous array indexing
            int[] drawables = {
                R.drawable.arrow, 
                R.drawable.tile, 
                R.drawable.glow, 
                R.drawable.back,
                R.drawable.home, 
                R.drawable.retry, 
                R.drawable.next, 
                R.drawable.play,
                R.drawable.paused, 
                R.drawable.settings, 
                R.drawable.sound_on,
                R.drawable.soundoff, 
                R.drawable.tick, 
                R.drawable.star,
                R.drawable.hint, 
                R.drawable.close, 
                R.drawable.lock, 
                R.drawable.share,
                R.drawable.level, 
                R.drawable.watchads, 
                R.drawable.removeads
            };

            for (int i = 0; i < drawables.length; i++) {
                try {
                    Bitmap bmp = BitmapFactory.decodeResource(getResources(), drawables[i]);
                    if (bmp != null) nativePushAsset(i, bmp);
                } catch (Exception e) {
                    android.util.Log.e("ResourceLoader", "Failed parsing bitmap array index: " + i);
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
            gestureDetector.onTouchEvent(event);
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

    public native void initNativeEngine(boolean systemDark);
    public native void nativePushAsset(int index, Bitmap bitmap);
    public native void nativeRender(Canvas canvas);
    public native void nativeOnTouch(float x, float y);
    public native void nativeOnResize(int w, int h);
    public native void nativeApplyScroll(float deltaY);
}
