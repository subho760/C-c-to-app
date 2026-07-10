package com.night.backgroundchange;

import androidx.appcompat.app.AppCompatActivity;
import android.media.MediaPlayer;
import android.os.Bundle;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.widget.FrameLayout;

public class MainActivity extends AppCompatActivity {
    static {
        System.loadLibrary("game_logic");
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
        
        // 1. Inflate the XML layout window immediately
        setContentView(R.layout.activity_main);

        // 2. Set up background sound assets safely
        try {
            clickPlayer = MediaPlayer.create(this, R.raw.click);
            winPlayer = MediaPlayer.create(this, R.raw.completelevel);
        } catch (Exception e) {
            // Audio layout fallback
        }

        // 3. Find your layout view container
        final FrameLayout container = findViewById(R.id.game_container);
        
        if (container != null) {
            // 4. Create the game engine view surface
            gameEngine = new GameEngine(this, this);
            container.addView(gameEngine);

            // 5. Initialize native level parameters right away
            int[] secureStarterGrid = new int[200]; 
            for (int i = 0; i < secureStarterGrid.length; i++) {
                secureStarterGrid[i] = 1; 
            }
            initNativeLevel(secureStarterGrid);

            // 6. 🟢 SAFE SYNCHRONIZATION: Listen for the hardware surface to be ready
            if (gameEngine instanceof SurfaceView) {
                SurfaceHolder holder = ((SurfaceView) gameEngine).getHolder();
                holder.addCallback(new SurfaceHolder.Callback() {
                    @Override
                    public void surfaceCreated(SurfaceHolder holder) {
                        isSurfaceReady = true;
                        // Start the drawing thread ONLY when the surface is physically built
                        if (gameEngine != null) {
                            gameEngine.resume();
                        }
                    }

                    @Override
                    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
                        // Responding to dimension switches if needed
                    }

                    @Override
                    public void surfaceDestroyed(SurfaceHolder holder) {
                        isSurfaceReady = false;
                        if (gameEngine != null) {
                            gameEngine.pause();
                        }
                    }
                });
            } else {
                // Fallback if GameEngine is a standard Custom View instead of a SurfaceView
                container.post(() -> {
                    isSurfaceReady = true;
                    gameEngine.resume();
                });
            }
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
        // Only resume if the hardware surface is ready to draw frames
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
