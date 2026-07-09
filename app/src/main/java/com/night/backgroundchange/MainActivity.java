package com.night.backgroundchange;

import androidx.appcompat.app.AppCompatActivity;
import android.app.AlertDialog;
import android.media.MediaPlayer;
import android.os.Bundle;
import android.widget.FrameLayout;

public class MainActivity extends AppCompatActivity {
    static {
        try {
            System.loadLibrary("game_logic");
        } catch (UnsatisfiedLinkError e) {
            // Caught if the C++ library fails to load entirely
        }
    }

    // Native methods declarations
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
        // 🟢 GLOBAL CRASH CATCHER: Displays errors directly on your screen
        Thread.setDefaultUncaughtExceptionHandler((thread, throwable) -> {
            runOnUiThread(() -> {
                new AlertDialog.Builder(MainActivity.this)
                    .setTitle("Runtime Crash Detected")
                    .setMessage(throwable.toString() + "\n\nAt: " + throwable.getStackTrace()[0].toString())
                    .setPositiveButton("Close", (dialog, which) -> finish())
                    .setCancelable(false)
                    .show();
            });
        });

        super.onCreate(savedInstanceState);
        
        // Match your original initialization order exactly
        gameEngine = new GameEngine(this, this);
        setContentView(R.layout.activity_main);

        FrameLayout container = findViewById(R.id.game_container);
        if (container != null && gameEngine != null) {
            container.addView(gameEngine);
        }

        clickPlayer = MediaPlayer.create(this, R.raw.click);
        winPlayer = MediaPlayer.create(this, R.raw.completelevel);
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
