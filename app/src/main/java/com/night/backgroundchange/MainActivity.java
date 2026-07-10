package com.night.backgroundchange;

// Changed from androidx.appcompat.app.AppCompatActivity to standard android.app.Activity
import android.app.Activity;
import android.media.MediaPlayer;
import android.os.Bundle;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.widget.FrameLayout;

public class MainActivity extends Activity {
    static {
        try {
            System.loadLibrary("game_logic");
        } catch (Throwable t) {
            // Prevent class-loading crashes if library is missing
        }
    }

    // Native JNI Bridges
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
        
        // 1. Inflate your activity layout window
        setContentView(R.layout.activity_main);

        // 2. Load background sound streams safely
        try {
            clickPlayer = MediaPlayer.create(this, R.raw.click);
            winPlayer = MediaPlayer.create(this, R.raw.completelevel);
        } catch (Exception e) {
            // Audio fail fallback
        }

        // 3. Bind the layout container to the GameEngine
        final FrameLayout container = findViewById(R.id.game_container);
        
        if (container != null) {
            gameEngine = new GameEngine(this, this);
            container.addView(gameEngine);

            // 4. Initialize native level parameters
            try {
                int[] secureStarterGrid = new int[200]; 
                for (int i = 0; i < secureStarterGrid.length; i++) {
                    secureStarterGrid[i] = 1; 
                }
                initNativeLevel(secureStarterGrid);
            } catch (Throwable nativeError) {
                // Prevent C++ bridge crashes
            }

            // 5. Safe Surface Synchronization Lifecycle
            if (gameEngine instanceof SurfaceView) {
                SurfaceHolder holder = ((SurfaceView) gameEngine).getHolder();
                holder.addCallback(new SurfaceHolder.Callback() {
                    @Override
                    public void surfaceCreated(SurfaceHolder holder) {
                        isSurfaceReady = true;
                        if (gameEngine != null) {
                            gameEngine.resume();
                        }
                    }

                    @Override
                    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {}

                    @Override
                    public void surfaceDestroyed(SurfaceHolder holder) {
                        isSurfaceReady = false;
                        if (gameEngine != null) {
                            gameEngine.pause();
                        }
                    }
                });
            } else {
                container.post(() -> {
                    isSurfaceReady = true;
                    if (gameEngine != null) {
                        gameEngine.resume();
                    }
                });
            }
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
