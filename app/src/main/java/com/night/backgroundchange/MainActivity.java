package com.night.backgroundchange;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.media.AudioAttributes;
import android.media.SoundPool;
import android.os.Bundle;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import androidx.appcompat.app.AppCompatActivity;

public class MainActivity extends AppCompatActivity implements SurfaceHolder.Callback {
    static { System.loadLibrary("native-lib"); }

    private SurfaceView surfaceView;
    private SoundPool soundPool;
    private int soundClick, soundComplete;
    private boolean isRunning = false;
    private Thread renderThread;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        
        // Fullscreen Immersive UI
        getWindow().getDecorView().setSystemUiVisibility(
            View.SYSTEM_UI_FLAG_LAYOUT_STABLE | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN |
            View.SYSTEM_UI_FLAG_FULLSCREEN | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION | 
            View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY);

        surfaceView = new SurfaceView(this);
        surfaceView.getHolder().addCallback(this);
        setContentView(surfaceView);

        initSoundPool();
        
        // Initialize NDK Engine
        boolean isDarkMode = getPreferences(Context.MODE_PRIVATE).getBoolean("dark_mode", true);
        initNativeEngine(isDarkMode);
        
        // Load Assets into NDK
        loadAssets();
    }

    private void initSoundPool() {
        AudioAttributes attrs = new AudioAttributes.Builder()
                .setUsage(AudioAttributes.USAGE_GAME)
                .setContentType(AudioAttributes.CONTENT_TYPE_SONIFICATION)
                .build();
        soundPool = new SoundPool.Builder().setMaxStreams(5).setAudioAttributes(attrs).build();
        soundClick = soundPool.load(this, R.raw.click, 1);
        soundComplete = soundPool.load(this, R.raw.completelevel, 1);
    }

    private void loadAssets() {
        int[] ids = {R.drawable.arrow, R.drawable.tile, R.drawable.glow, R.drawable.play, 
                     R.drawable.retry, R.drawable.star, R.drawable.next};
        String[] names = {"arrow", "tile", "glow", "play", "retry", "star", "next"};
        for (int i = 0; i < ids.length; i++) {
            Bitmap bmp = BitmapFactory.decodeResource(getResources(), ids[i]);
            if (bmp != null) {
                nativePushAsset(names[i], bmp);
            }
        }
    }

    // Called by NDK to trigger SFX
    public void playSound(int type) {
        int soundId = (type == 0) ? soundClick : soundComplete;
        soundPool.play(soundId, 1.0f, 1.0f, 0, 0, 1.0f);
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        if (event.getAction() == MotionEvent.ACTION_DOWN) {
            nativeOnTouch(event.getX(), event.getY());
        }
        return true;
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        isRunning = true;
        renderThread = new Thread(() -> {
            while (isRunning) {
                android.graphics.Canvas canvas = holder.lockCanvas();
                if (canvas != null) {
                    // Create a bitmap that matches the canvas size for native rasterizing
                    Bitmap frameBitmap = Bitmap.createBitmap(canvas.getWidth(), canvas.getHeight(), Bitmap.Config.ARGB_8888);
                    nativeRender(frameBitmap);
                    canvas.drawBitmap(frameBitmap, 0, 0, null);
                    holder.unlockCanvasAndPost(canvas);
                }
            }
        });
        renderThread.start();
    }

    @Override
    public void surfaceChanged(SurfaceHolder h, int f, int w, int h1) {
        nativeOnResize(w, h1);
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder h) {
        isRunning = false;
        try { renderThread.join(); } catch (InterruptedException e) { e.printStackTrace(); }
    }

           //  FIXED CODE FOR THE BOTTOM OF THE FILE
    public native void initNativeEngine(boolean darkTheme);
    public native void nativePushAsset(String name, Object bitmap);
    public native void nativeRender(Object canvas); // <-- Changed from Canvas to Object
    public native void nativeOnTouch(float x, float y);
    public native void nativeOnResize(int w, int h);
}
