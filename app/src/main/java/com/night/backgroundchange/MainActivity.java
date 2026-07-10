package com.night.backgroundchange;

import androidx.appcompat.app.AppCompatActivity;
import android.media.MediaPlayer;
import android.os.Bundle;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.widget.FrameLayout;
import android.widget.TextView;
import android.graphics.Color;

public class MainActivity extends AppCompatActivity {
    static {
        System.loadLibrary("game_logic");
    }

    public native String stringFromJNI();
    public native void initNativeLevel(int[] data);
    public native boolean canArrowMove(int arrowId);
    public native void removeNativeArrow(int arrowId);

    private GameEngine gameEngine;
    private MediaPlayer clickPlayer;
    private MediaPlayer winPlayer;
    private boolean soundEnabled = true;
    private boolean isSurfaceReady = false;
    
    private TextView statusTracker;
    private StringBuilder statusLog = new StringBuilder();

    private void logStep(String message) {
        runOnUiThread(() -> {
            statusLog.append("✔ ").append(message).append("\n");
            if (statusTracker != null) {
                statusTracker.setText(statusLog.toString());
            }
        });
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        
        // 1. Create a clean tracking overlay layout frame
        FrameLayout mainLayout = new FrameLayout(this);
        mainLayout.setBackgroundColor(Color.BLACK);

        statusTracker = new TextView(this);
        statusTracker.setTextColor(Color.GREEN);
        statusTracker.setTextSize(14);
        statusTracker.setPadding(40, 80, 40, 40);
        statusTracker.setText("Starting detailed GameEngine checks...\n");
        
        FrameLayout gameContainer = new FrameLayout(this);
        mainLayout.addView(gameContainer);
        mainLayout.addView(statusTracker); 
        setContentView(mainLayout);

        logStep("MainActivity started safely.");

        // 2. Test JNI Connection
        try {
            String jniCheck = stringFromJNI();
            logStep("Native JNI Handshake: " + jniCheck);
        } catch (Throwable t) {
            logStep("JNI Handshake skipped: " + t.getMessage());
        }

        // 3. Run the GameEngine creation inside a monitored UI runner sequence
        runOnUiThread(() -> {
            try {
                logStep("Attempting to run GameEngine constructor...");
                
                // If this line freezes, we need to look into GameEngine.java file layout
                gameEngine = new GameEngine(MainActivity.this, MainActivity.this);
                
                logStep("GameEngine instance created successfully!");
                gameContainer.addView(gameEngine);
                logStep("GameEngine view added to container frame.");

                // 4. Initialize level data mapping arrays
                int[] secureStarterGrid = new int[200]; 
                for (int i = 0; i < secureStarterGrid.length; i++) {
                    secureStarterGrid[i] = 1; 
                }
                initNativeLevel(secureStarterGrid);
                logStep("Native level data matrix synced safely.");

                // 5. Setup View callbacks
                if (gameEngine instanceof SurfaceView) {
                    logStep("SurfaceView detected. Assigning hardware holder hooks...");
                    SurfaceHolder holder = ((SurfaceView) gameEngine).getHolder();
                    holder.addCallback(new SurfaceHolder.Callback() {
                        @Override
                        public void surfaceCreated(SurfaceHolder holder) {
                            isSurfaceReady = true;
                            logStep("Surface created! Resuming engine thread loop...");
                            gameEngine.resume();
                        }

                        @Override
                        public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {}

                        @Override
                        public void surfaceDestroyed(SurfaceHolder holder) {
                            isSurfaceReady = false;
                            gameEngine.pause();
                        }
                    });
                } else {
                    logStep("Standard view wrapper detected. Directing thread startup sequence...");
                    isSurfaceReady = true;
                    gameEngine.resume();
                    logStep("Engine thread resume loop executed.");
                }

            } catch (Throwable e) {
                logStep("🚨 CRASH INSIDE ENGINE CORNER: " + e.toString());
                if (e.getCause() != null) {
                    logStep("REASON: " + e.getCause().toString());
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
