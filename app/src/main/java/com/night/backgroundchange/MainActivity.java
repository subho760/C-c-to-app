package com.night.backgroundchange;

import androidx.appcompat.app.AppCompatActivity;
import android.media.MediaPlayer;
import android.os.Bundle;
import android.view.ViewTreeObserver;
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
    private boolean isEngineInitialized = false;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        
        // 1. Inflate the layout instantly
        setContentView(R.layout.activity_main);

        // 2. Set up sound streams safely
        try {
            clickPlayer = MediaPlayer.create(this, R.raw.click);
            winPlayer = MediaPlayer.create(this, R.raw.completelevel);
        } catch (Exception e) {
            // Audio layout fallback
        }

        // 3. Obtain the layout view container safely
        final FrameLayout container = findViewById(R.id.game_container);
        
        if (container != null) {
            // 🟢 FORCE ANDROID TO WAIT UNTIL THE SURFACE HAS REAL, NON-ZERO DIMENSIONS
            container.getViewTreeObserver().addOnGlobalLayoutListener(new ViewTreeObserver.OnGlobalLayoutListener() {
                @Override
                public void onGlobalLayout() {
                    // Prevent duplicate initializations when the layout refreshes
                    if (isEngineInitialized) return;

                    int width = container.getWidth();
                    int height = container.getHeight();

                    // Only proceed if the layout container has been allocated real screen space
                    if (width > 0 && height > 0) {
                        isEngineInitialized = true;

                        // Create the engine view with confirmed safe dimensions
                        gameEngine = new GameEngine(MainActivity.this, MainActivity.this);
                        container.addView(gameEngine);

                        // Pass your level setup array downward to the C++ layer
                        int[] startingLevelData = {1, 0, 1, 0, 1}; 
                        initNativeLevel(startingLevelData);
                        
                        // Explicitly kickstart the drawing engine loop thread
                        gameEngine.resume();

                        // Remove listener safely to preserve device memory performance
                        container.getViewTreeObserver().removeOnGlobalLayoutListener(this);
                    }
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
        // Only resume if the engine has been fully initialized by the layout listener
        if (gameEngine != null && isEngineInitialized) {
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
