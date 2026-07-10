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
            // Wait until the layout container has real dimensions assigned
            container.getViewTreeObserver().addOnGlobalLayoutListener(new ViewTreeObserver.OnGlobalLayoutListener() {
                @Override
                public void onGlobalLayout() {
                    if (isEngineInitialized) return;

                    int width = container.getWidth();
                    int height = container.getHeight();

                    if (width > 0 && height > 0) {
                        isEngineInitialized = true;

                        // Send a reliable initial allocation matrix size to the JNI layer
                        int[] secureStarterGrid = new int[200]; 
                        for (int i = 0; i < secureStarterGrid.length; i++) {
                            secureStarterGrid[i] = 1; 
                        }
                        initNativeLevel(secureStarterGrid);

                        // Create the engine view with verified layout dimensions
                        gameEngine = new GameEngine(MainActivity.this, MainActivity.this);
                        container.addView(gameEngine);
                        
                        // Kickstart drawing engine loops
                        gameEngine.resume();

                        // Safely discard the layout listener callback loop
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
