package com.night.backgroundchange;

import androidx.appcompat.app.AppCompatActivity;
import android.media.MediaPlayer;
import android.os.Bundle;
import android.widget.FrameLayout;

public class MainActivity extends AppCompatActivity {
    static {
        System.loadLibrary("game_logic");
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

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        
        // 1. Inflate layout first
        setContentView(R.layout.activity_main);

        // 2. Setup your media assets safely
        try {
            clickPlayer = MediaPlayer.create(this, R.raw.click);
            winPlayer = MediaPlayer.create(this, R.raw.completelevel);
        } catch (Exception e) {
            // Audio bypass layout fallback
        }

        // 3. Find the container view window
        final FrameLayout container = findViewById(R.id.game_container);
        
        if (container != null) {
            // 🟢 FORCE ANDROID TO WAIT UNTIL THE SCREEN IS FULLY DRAWN AND MEASURED
            container.post(new Runnable() {
                @Override
                public void run() {
                    // Create and add the game engine ONLY when the surface dimensions are ready
                    gameEngine = new GameEngine(MainActivity.this, MainActivity.this);
                    container.addView(gameEngine);

                    // Initialize your native map structures safely inside the rendering loop
                    int[] startingLevelData = {1, 0, 1, 0, 1}; 
                    initNativeLevel(startingLevelData);
                    
                    // Resume engine processing explicitly
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
        if (gameEngine != null) gameEngine.resume();
    }

    @Override
    protected void onPause() {
        super.onPause();
        if (gameEngine != null) gameEngine.pause();
    }
}
