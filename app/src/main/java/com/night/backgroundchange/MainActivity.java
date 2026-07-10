package com.night.backgroundchange;

import androidx.appcompat.app.AppCompatActivity;
import android.media.MediaPlayer;
import android.os.Bundle;
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

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        
        // 1. Inflate layout first so screen dimensions (width/height) are active
        setContentView(R.layout.activity_main);

        // 2. Safely initialize the game engine view with the active screen context
        gameEngine = new GameEngine(this, this);

        // 3. Find and inject the engine into your layout container XML window
        FrameLayout container = findViewById(R.id.game_container);
        if (container != null) {
            container.addView(gameEngine);
        }

        // 4. Initialize media files safely
        try {
            clickPlayer = MediaPlayer.create(this, R.raw.click);
            winPlayer = MediaPlayer.create(this, R.raw.completelevel);
        } catch (Exception e) {
            // Audio setup fallback
        }

        // 5. Send initial starter level data array down to C++ layer
        int[] startingLevelData = {1, 0, 1, 0, 1}; 
        initNativeLevel(startingLevelData);
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
        if (gameEngine != null) gameEngine.resume();
    }

    @Override
    protected void onPause() {
        super.onPause();
        if (gameEngine != null) gameEngine.pause();
    }
}
