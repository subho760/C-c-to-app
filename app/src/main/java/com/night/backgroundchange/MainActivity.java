package com.night.backgroundchange;

import android.app.Activity;
import android.media.MediaPlayer;
import android.os.Bundle;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.widget.FrameLayout;

public class MainActivity extends Activity {
    static {
        try {
            System.loadLibrary("game_logic");
        } catch (Throwable t) {
            // Prevent crashes if the library isn't found
        }
    }

    // Native JNI definitions
    public native String stringFromJNI();
    public native void initNativeLevel(int[] data);
    public native boolean canArrowMove(int arrowId);
    public native void removeNativeArrow(int arrowId);

    private GameEngine gameEngine;
    private MediaPlayer clickPlayer;
    private MediaPlayer winPlayer;
    private boolean soundEnabled = true;
    private boolean isSurfaceReady = false;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        
        // 1. Create a raw container frame entirely in code (bypassing XML requirements)
        FrameLayout rootContainer = new FrameLayout(this);
        setContentView(rootContainer);

        // 2. Safely initialize the game engine view surface
        gameEngine = new GameEngine(this, this);
        rootContainer.addView(gameEngine);

        // 3. Set up background sound assets inside clean error catches
        try {
            clickPlayer = MediaPlayer.create(this, R.raw.click);
            winPlayer = MediaPlayer.create(this, R.raw.completelevel);
        } catch (Exception e) {
            // Safe fallback if audio resources are missing
        }

        // 4. Initialize native level parameters right away
        try {
            int[] secureStarterGrid = new int[200]; 
            for (int i = 0; i < secureStarterGrid.length; i++) {
                secureStarterGrid[i] = 1; 
            }
            initNativeLevel(secureStarterGrid);
        } catch (Throwable nativeError) {
            // Safety guard for C++ bridge allocations
        }

        // 5. Explicitly handle surface generation lifecycle states
        if (gameEngine instanceof SurfaceView) {
            SurfaceHolder holder = ((SurfaceView) gameEngine).getHolder();
            holder.addCallback(new SurfaceHolder.Callback() {
                @Override
                public void surfaceCreated(SurfaceHolder holder) {
                    isSurfaceReady = true;
                    if (gameEngine != null) {
                        gameEngine.resume();
                    }
                }

                @Override
                public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {}

                @Override
                public void surfaceDestroyed(SurfaceHolder holder) {
                    isSurfaceReady = false;
                    if (gameEngine != null) {
                        gameEngine.pause();
                    }
                }
            });
        } else {
            rootContainer.post(() -> {
                isSurfaceReady = true;
                if (gameEngine != null) {
                    gameEngine.resume();
                }
            });
        }
    }

    public void playSound(boolean isWin) {
        if (!soundEnabled) return;
        if (isWin) {
            if (winPlayer != null) winPlayer.start();
        } else {
            if (clickPlayer != null) clickPlayer.start();
        }
    }

    public void onLevelComplete() {
        playSound(true);
        runOnUiThread(() -> {
            if (gameEngine != null) {
                gameEngine.loadLevel(2);
            }
        });
    }

    @Override
    protected void onResume() {
        super.onResume();
        if (gameEngine != null && isSurfaceReady) {
            gameEngine.resume();
        }
    }

    @Override
    protected void onPause() {
        super.onPause();
        if (gameEngine != null) {
            gameEngine.pause();
        }
    }
}
