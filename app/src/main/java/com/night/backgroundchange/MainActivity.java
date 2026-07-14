package com.night.backgroundchange;

import android.content.Context;
import android.content.SharedPreferences;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.media.AudioAttributes;
import android.media.SoundPool;
import android.os.Bundle;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import androidx.appcompat.app.AppCompatActivity;

public class MainActivity extends AppCompatActivity {
    private GameView gameView;
    private SoundPool soundPool;
    private int clickSound = -1;
    private int completeSound = -1;
    private boolean soundEnabled = true;

    static {
        try {
            System.loadLibrary("native-lib");
        } catch (UnsatisfiedLinkError e) {
            android.util.Log.e("NativeGame", "Failed to load native-lib library binary file", e);
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        
        getWindow().getDecorView().setSystemUiVisibility(
                View.SYSTEM_UI_FLAG_FULLSCREEN | 
                View.SYSTEM_UI_FLAG_HIDE_NAVIGATION | 
                View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY);

        SharedPreferences prefs = getPreferences(Context.MODE_PRIVATE);
        soundEnabled = prefs.getBoolean("sound", true);
        boolean isDarkMode = prefs.getBoolean("dark_mode", true);

        initSounds();
        gameView = new GameView(this, isDarkMode);
        setContentView(gameView);
    }

    private void initSounds() {
        try {
            AudioAttributes attrs = new AudioAttributes.Builder()
                    .setUsage(AudioAttributes.USAGE_GAME)
                    .setContentType(AudioAttributes.CONTENT_TYPE_SONIFICATION)
                    .build();
            soundPool = new SoundPool.Builder().setMaxStreams(5).setAudioAttributes(attrs).build();
            
            int clickId = getResources().getIdentifier("click", "raw", getPackageName());
            int completeId = getResources().getIdentifier("completelevel", "raw", getPackageName());
            
            if (clickId != 0) clickSound = soundPool.load(this, clickId, 1);
            if (completeId != 0) completeSound = soundPool.load(this, completeId, 1);
        } catch (Exception e) {
            android.util.Log.e("NativeGame", "SoundPool failed setup. Silent fallback mode.", e);
            soundEnabled = false;
        }
    }

    public void playSound(int type) {
        if (!soundEnabled || soundPool == null) return;
        try {
            if (type == 0 && clickSound != -1) {
                soundPool.play(clickSound, 1.0f, 1.0f, 0, 0, 1.0f);
            } else if (type == 1 && completeSound != -1) {
                soundPool.play(completeSound, 1.0f, 1.0f, 0, 0, 1.0f);
            }
        } catch (Exception e) {
            android.util.Log.e("NativeGame", "Audio processing trace tracking fault.", e);
        }
    }

    class GameView extends SurfaceView implements SurfaceHolder.Callback, Runnable {
        private Thread gameThread;
        private volatile boolean running;
        private final SurfaceHolder holder;

        public GameView(Context context, boolean darkTheme) {
            super(context);
            holder = getHolder();
            holder.addCallback(this);
            
            int[] ids = {R.drawable.arrow, R.drawable.tile, R.drawable.glow, R.drawable.back, 
                         R.drawable.home, R.drawable.retry, R.drawable.next, R.drawable.play,
                         R.drawable.paused, R.drawable.settings, R.drawable.sound_on, 
                         R.drawable.soundoff, R.drawable.tick, R.drawable.star, 
                         R.drawable.hint, R.drawable.close, R.drawable.lock};
            
            try {
                initNativeEngine(darkTheme);
            } catch (Exception e) {
                android.util.Log.e("NativeGame", "Engine loop instantiation fault", e);
            }
            
            for(int i = 0; i < ids.length; i++) {
                try {
                    Bitmap bmp = BitmapFactory.decodeResource(getResources(), ids[i]);
                    if (bmp != null) {
                        nativePushAsset(i, bmp);
                    }
                } catch (Exception e) {
                    android.util.Log.e("GameAssets", "Failed to load resource ID index: " + i, e);
                }
            }
        }

        @Override
        public void surfaceCreated(SurfaceHolder holder) {
            running = true;
            gameThread = new Thread(this);
            gameThread.start();
        }

        @Override
        public void run() {
            while (running) {
                if (!holder.getSurface().isValid()) continue;
                Canvas canvas = holder.lockCanvas();
                if (canvas != null) {
                    try {
                        nativeRender(canvas);
                    } catch (Exception e) {
                        android.util.Log.e("NativeGame", "Frame canvas exception drop.", e);
                    }
                    holder.unlockCanvasAndPost(canvas);
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
                try {
                    nativeOnTouch(event.getX(), event.getY());
                } catch (Exception e) {
                    android.util.Log.e("NativeGame", "Input processing error.", e);
                }
            }
            return true;
        }

        @Override
        public void surfaceChanged(SurfaceHolder h, int f, int w, int h1) {
            try {
                nativeOnResize(w, h1);
            } catch (Exception e) {
                android.util.Log.e("NativeGame", "Resize lifecycle exception.", e);
            }
        }
        
        @Override
        public void surfaceDestroyed(SurfaceHolder h) {
            running = false;
            try { 
                if (gameThread != null) gameThread.join(); 
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
