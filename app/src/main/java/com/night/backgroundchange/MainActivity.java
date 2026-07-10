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
            // Safe fallback if C++ library load issues happen
        }
    }

    // Native JNI connections
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
        
        // 1. Create a dynamic layout container right in the code
        FrameLayout rootContainer = new FrameLayout(this);
        setContentView(rootContainer);

        // 2. Instantiate GameEngine passing the direct view container reference
        gameEngine = new GameEngine(this, this, rootContainer);
        rootContainer.addView(gameEngine);

        // 3. Load background sound streams safely
        try {
            clickPlayer = MediaPlayer.create(this, R.raw.click);
            winPlayer = MediaPlayer.create(this, R.raw.completelevel);
        } catch (Exception e) {
            // Audio layout fallback
        }

        // 4. Send the starter 200-element matrix configuration down to C++ layer
        try {
            int[] secureStarterGrid = new int[200]; 
            for (int i = 0; i < secureStarterGrid.length; i++) {
                secureStarterGrid[i] = 1; 
            }
            initNativeLevel(secureStarterGrid);
        } catch (Throwable nativeError) {
            // Protect native call bridges
        }

        // 5. Hardware Drawing Surface Lifecycle Hook Sync
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
            gameThreadPause();
        }
    }

    private void gameThreadPause() {
        if (gameEngine != null) {
            gameEngine.pause();
        }
    }
}
