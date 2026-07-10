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
        
        // 1. Inflate layout first to ensure view instances exist
        setContentView(R.layout.activity_main);

        // 2. Setup media assets inside safety catch blocks
        try {
            clickPlayer = MediaPlayer.create(this, R.raw.click);
            winPlayer = MediaPlayer.create(this, R.raw.completelevel);
        } catch (Exception e) {
            // Audio layout bypass logic
        }

        // 3. Bind view rendering components safely
        final FrameLayout container = findViewById(R.id.game_container);
        
        if (container != null) {
            container.getViewTreeObserver().addOnGlobalLayoutListener(new ViewTreeObserver.OnGlobalLayoutListener() {
                @Override
                public void onGlobalLayout() {
                    if (isEngineInitialized) return;

                    int width = container.getWidth();
                    int height = container.getHeight();

                    if (width > 0 && height > 0) {
                        isEngineInitialized = true;

                        // 🟢 INITIALIZE NATIVE MATRIX MEMORY BEFORE CREATING GAME VIEWS
                        // Provide a comprehensive safe initial level grid map array to prevent engine reading mismatch 
                        int[] secureStarterGrid = new int[200]; 
                        for(int i = 0; i < secureStarterGrid.length; i++) {
                            secureStarterGrid[i] = 1; // Populate with generic block item data values
                        }
                        initNativeLevel(secureStarterGrid);

                        // Now create the drawing engine safely
                        gameEngine = new GameEngine(MainActivity.this, MainActivity.this);
                        container.addView(gameEngine);
                        
                        // Force thread activation
                        gameEngine.resume();

                        // Clean layout listeners safely
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
cn
