package com.night.backgroundchange;

import android.app.Activity;
import android.media.MediaPlayer;
import android.os.Bundle;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.widget.Button;
import android.widget.FrameLayout;
import android.graphics.Color;

public class MainActivity extends Activity {
    static {
        try {
            System.loadLibrary("game_logic");
        } catch (Throwable t) {
            // Prevent initialization load failures from crashing the class tracker
        }
    }

    // Native C++ methods
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
        
        // 1. Create a pure programmatic layout container
        final FrameLayout rootContainer = new FrameLayout(this);
        rootContainer.setBackgroundColor(Color.parseColor("#121212")); // Safe Dark Background
        setContentView(rootContainer);

        // 2. Add a fallback diagnostic layout button
        final Button startButton = new Button(this);
        startButton.setText("🚀 START NIGHT SHADOW GAME ENGINE");
        startButton.setBackgroundColor(Color.DKGRAY);
        startButton.setTextColor(Color.WHITE);
        
        // Position button cleanly in the middle of the screen
        FrameLayout.LayoutParams btnParams = new FrameLayout.LayoutParams(
                FrameLayout.LayoutParams.WRAP_CONTENT,
                FrameLayout.LayoutParams.WRAP_CONTENT
        );
        btnParams.gravity = android.view.Gravity.CENTER;
        startButton.setLayoutParams(btnParams);
        rootContainer.addView(startButton);

        // 3. Audio asset loader wrapper
        try {
            clickPlayer = MediaPlayer.create(this, R.raw.click);
            winPlayer = MediaPlayer.create(this, R.raw.completelevel);
        } catch (Exception e) {
            // Keep going if files are absent
        }

        // 4. Fire up the game engine ONLY when the button is clicked!
        startButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                try {
                    // Hide the button so the engine can draw fully
                    startButton.setVisibility(View.GONE);

                    // Initialize game layout structures dynamically
                    gameEngine = new GameEngine(MainActivity.this, MainActivity.this);
                    rootContainer.addView(gameEngine);

                    // Initialize JNI level data array matrix down in C++ logic
                    int[] secureStarterGrid = new int[200]; 
                    for (int i = 0; i < secureStarterGrid.length; i++) {
                        secureStarterGrid[i] = 1; 
                    }
                    initNativeLevel(secureStarterGrid);

                    // Synchronize surface view callback handlers
                    if (gameEngine instanceof SurfaceView) {
                        SurfaceHolder holder = ((SurfaceView) gameEngine).getHolder();
                        holder.addCallback(new SurfaceHolder.Callback() {
                            @Override
                            public void surfaceCreated(SurfaceHolder holder) {
                                isSurfaceReady = true;
                                if (gameEngine != null) gameEngine.resume();
                            }

                            @Override
                            public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {}

                            @Override
                            public void surfaceDestroyed(SurfaceHolder holder) {
                                isSurfaceReady = false;
                                if (gameEngine != null) gameEngine.pause();
                            }
                        });
                    } else {
                        rootContainer.post(() -> {
                            isSurfaceReady = true;
                            if (gameEngine != null) gameEngine.resume();
                        });
                    }
                } catch (Throwable engineCrash) {
                    // If clicking the button crashes the app, the issue is 100% inside GameEngine.java
                    startButton.setVisibility(View.VISIBLE);
                    startButton.setText("❌ Error in GameEngine! " + engineCrash.getMessage());
                }
            }
        });
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
