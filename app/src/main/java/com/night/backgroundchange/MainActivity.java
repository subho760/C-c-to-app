package com.night.backgroundchange;

import android.app.Activity;
import android.media.MediaPlayer;
import android.os.Bundle;
import android.view.View;
import android.widget.FrameLayout;

    public class MainActivity extends AppCompatActivity {
    static {
        // Replace "game_logic" with the actual library name from CMakeLists.txt
        System.loadLibrary("game_logic"); 
    }
    
    // Your native methods declaration...
    public native String stringFromJNI();
    }
    

    private GameEngine gameEngine;
    private MediaPlayer clickPlayer, winPlayer;
    private boolean soundEnabled = true;

    // Native methods
    public native void initNativeLevel(int[] data);
    public native boolean canArrowMove(int arrowId);
    public native void removeNativeArrow(int arrowId);

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        
        gameEngine = new GameEngine(this, this);
        setContentView(R.layout.activity_main);
        
        // Add game engine to layout
        FrameLayout container = findViewById(R.id.game_container);
        container.addView(gameEngine);

        clickPlayer = MediaPlayer.create(this, R.raw.click);
        winPlayer = MediaPlayer.create(this, R.raw.completelevel);
    }

    public void playSound(boolean isWin) {
        if (!soundEnabled) return;
        if (isWin) winPlayer.start();
        else clickPlayer.start();
    }

    public void onLevelComplete() {
        playSound(true);
        runOnUiThread(() -> {
            // Show Win UI (Next Level, Stars)
            // For now, auto-load next level
            gameEngine.loadLevel(2); 
        });
    }

    @Override
    protected void onResume() { super.onResume(); gameEngine.resume(); }
    @Override
    protected void onPause() { super.onPause(); gameEngine.pause(); }
}
