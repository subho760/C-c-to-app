package com.night.backgroundchange;

import androidx.appcompat.app.AppCompatActivity;
import android.os.Bundle;
import android.widget.FrameLayout;
import android.widget.TextView;
import android.graphics.Color;
import android.media.MediaPlayer;

public class MainActivity extends AppCompatActivity {
    static {
        try {
            System.loadLibrary("game_logic");
        } catch (Throwable t) {
            // Handled inside onCreate safely
        }
    }

    public native String stringFromJNI();
    public native void initNativeLevel(int[] data);
    public native boolean canArrowMove(int arrowId);
    public native void removeNativeArrow(int arrowId);

    private GameEngine gameEngine;
    private MediaPlayer clickPlayer;
    private MediaPlayer winPlayer;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // 1. Create a clean, simple text window to show us what happens
        TextView errorDisplay = new TextView(this);
        errorDisplay.setTextColor(Color.WHITE);
        errorDisplay.setTextSize(16);
        errorDisplay.setPadding(50, 100, 50, 50);
        errorDisplay.setText("Checking app files... Please wait.");
        setContentView(errorDisplay);

        try {
            // 2. Load layout files safely
            setContentView(R.layout.activity_main);
            
            FrameLayout container = findViewById(R.id.game_container);
            if (container == null) {
                errorDisplay.setText("❌ Error: Could not find 'game_container' in activity_main.xml layout file.");
                setContentView(errorDisplay);
                return;
            }

            // 3. Try to start the GameEngine
            try {
                gameEngine = new GameEngine(this, this);
                container.addView(gameEngine);
            } catch (Throwable engineError) {
                errorDisplay.setText("❌ The app is freezing inside GameEngine.java!\n\nDetails:\n" + engineError.toString());
                setContentView(errorDisplay);
                return;
            }

            // 4. Set up audio files safely
            try {
                clickPlayer = MediaPlayer.create(this, R.raw.click);
                winPlayer = MediaPlayer.create(this, R.raw.completelevel);
            } catch (Exception e) {
                // Keep moving if sound files are missing
            }

            // 5. Fire up the game loop
            if (gameEngine != null) {
                gameEngine.resume();
            }

        } catch (Throwable overallError) {
            errorDisplay.setText("❌ App Setup Failed!\n\nError Message:\n" + overallError.toString());
            setContentView(errorDisplay);
        }
    }

    public void playSound(boolean isWin) {
        if (isWin && winPlayer != null) winPlayer.start();
        if (!isWin && clickPlayer != null) clickPlayer.start();
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
        if (gameEngine != null) {
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
