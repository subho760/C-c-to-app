package com.night.backgroundchange;

import android.content.Context;
import android.content.SharedPreferences;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas; // 👈 Explicitly imported to avoid compile issues
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
    private int clickSound, completeSound;
    private boolean soundEnabled = true;

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

        SharedPreferences prefs = getPreferences(Context.MODE_PRIVATE);
        soundEnabled = prefs.getBoolean("sound", true);
        boolean isDarkMode = prefs.getBoolean("dark_mode", true);

        initSounds();
        gameView = new GameView(this, isDarkMode);
        setContentView(gameView);
    }

    private void initSounds() {
        AudioAttributes attrs = new AudioAttributes.Builder()
                .setUsage(AudioAttributes.USAGE_GAME)
                .setContentType(AudioAttributes.CONTENT_TYPE_SONIFICATION)
                .build();
        soundPool = new SoundPool.Builder().setMaxStreams(5).setAudioAttributes(attrs).build();
        clickSound = soundPool.load(this, R.raw.click, 1);
        completeSound = soundPool.load(this, R.raw.completelevel, 1);
    }

    public void playSound(int type) {
        if (!soundEnabled) return;
        if (type == 0) {
            soundPool.play(clickSound, 1.0f, 1.0f, 0, 0, 1.0f);
        } else {
            soundPool.play(completeSound, 1.0f, 1.0f, 0, 0, 1.0f);
        }
    }

    class GameView extends SurfaceView implements SurfaceHolder.Callback, Runnable {
        private Thread gameThread;
        private boolean running;
        private SurfaceHolder holder;

        public GameView(Context context, boolean darkTheme) {
            super(context);
            holder = getHolder();
            holder.addCallback(this);
            
            int[] ids = {R.drawable.arrow, R.drawable.tile, R.drawable.glow, R.drawable.back, 
                         R.drawable.home, R.drawable.retry, R.drawable.next, R.drawable.play,
                         R.drawable.paused, R.drawable.settings, R.drawable.sound_on, 
                         R.drawable.soundoff, R.drawable.tick, R.drawable.star, 
                         R.drawable.hint, R.drawable.close, R.drawable.lock};
                         
            String[] names = {"arrow", "tile", "glow", "back", "home", "retry", "next", "play", 
                              "paused", "settings", "sound_on", "soundoff", "tick", "star", 
                              "hint", "close", "lock"};
            
            initNativeEngine(darkTheme);
            
            for(int i = 0; i < ids.length; i++) {
                try {
                    Bitmap bmp = BitmapFactory.decodeResource(getResources(), ids[i]);
                    if (bmp != null) {
                        nativePushAsset(names[i], bmp);
                    }
                } catch (Exception e) {
                    android.util.Log.e("GameAssets", "Failed to load resource: " + names[i], e);
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
                    nativeRender(canvas);
                    holder.unlockCanvasAndPost(canvas);
                }
                try {
                    Thread.sleep(16);
                } catch (InterruptedException e) {
                    e.printStackTrace();
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
        public void surfaceChanged(SurfaceHolder h, int f, int w, int h1) {
            nativeOnResize(w, h1);
        }
        
        @Override
        public void surfaceDestroyed(SurfaceHolder h) {
            running = false;
            try { 
                if (gameThread != null) gameThread.join(); 
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }
    }

    // Cleaned JNI Signatures
    public native void initNativeEngine(boolean darkTheme);
    public native void nativePushAsset(String name, Bitmap bitmap);
    public native void nativeRender(Canvas canvas);
    public native void nativeOnTouch(float x, float y);
    public native void nativeOnResize(int w, int h);
}
