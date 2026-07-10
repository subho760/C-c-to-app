package com.night.backgroundchange;

import androidx.appcompat.app.AppCompatActivity;
import android.media.MediaPlayer;
import android.os.Bundle;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.widget.FrameLayout;
import android.widget.TextView;
import android.widget.ScrollView;
import android.graphics.Color;

public class MainActivity extends AppCompatActivity {
    static {
        try {
            System.loadLibrary("game_logic");
        } catch (Throwable e) {
            // Handled directly inside onCreate diagnostics
        }
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
    private boolean isSurfaceReady = false;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        // 1. 🟢 THE ON-SCREEN CRASH DIAGNOSTIC LOGGER ENGINE
        Thread.setDefaultUncaughtExceptionHandler((thread, throwable) -> {
            runOnUiThread(() -> {
                ScrollView scrollView = new ScrollView(MainActivity.this);
                scrollView.setBackgroundColor(Color.parseColor("#2A0000"));
                scrollView.setPadding(30, 50, 30, 50);

                TextView errorText = new TextView(MainActivity.this);
                errorText.setTextColor(Color.RED);
                errorText.setTextSize(16);
                errorText.setTypeface(android.graphics.Typeface.MONOSPACE);

                StringBuilder sb = new StringBuilder();
                sb.append("🚨 --- ACTIVE RUNTIME EXCEPTION CAUGHT ---\n\n");
                sb.append("ERROR TYPE:\n").append(throwable.toString()).append("\n\n");
                
                if (throwable.getStackTrace().length > 0) {
                    sb.append("EXACT LOCATION OF FAILURE:\n")
                      .append("File: ").append(throwable.getStackTrace()[0].getFileName()).append("\n")
                      .append("Class: ").append(throwable.getStackTrace()[0].getClassName()).append("\n")
                      .append("Method: ").append(throwable.getStackTrace()[0].getMethodName()).append("\n")
                      .append("Line Number: ").append(throwable.getStackTrace()[0].getLineNumber()).append("\n\n");
                }

                sb.append("FULL STACK TRACE LOG:\n");
                for (StackTraceElement element : throwable.getStackTrace()) {
                    sb.append("   at ").append(element.toString()).append("\n");
                }

                errorText.setText(sb.toString());
                scrollView.addView(errorText);
                setContentView(scrollView);
            });
        });

        super.onCreate(savedInstanceState);

        // Try-catch block specifically to detect missing C++ library linkages on launch
        try {
            stringFromJNI();
        } catch (UnsatisfiedLinkError jniError) {
            throw new RuntimeException("C++ Shared Library Linkage Error! The game engine methods do not match the compiled C++ code structures.", jniError);
        }

        // 2. Inflate the XML layout window immediately
        try {
            setContentView(R.layout.activity_main);
        } catch (Throwable layoutError) {
            throw new RuntimeException("Layout Inflation Failed! Check your activity_main.xml layout configuration file.", layoutError);
        }

        // 3. Set up background sound assets safely
        try {
            clickPlayer = MediaPlayer.create(this, R.raw.click);
            winPlayer = MediaPlayer.create(this, R.raw.completelevel);
        } catch (Exception e) {
            // Safe fallback if audio is missing
        }

        // 4. Find your layout view container
        final FrameLayout container = findViewById(R.id.game_container);
        
        if (container == null) {
            throw new RuntimeException("Missing View ID! Could not find R.id.game_container inside your activity_main.xml file.");
        }

        // 5. Create and attach the game engine view surface
        try {
            gameEngine = new GameEngine(this, this);
            container.addView(gameEngine);
        } catch (Throwable engineError) {
            throw new RuntimeException("Crash inside GameEngine Constructor initialization wrapper!", engineError);
        }

        // 6. Initialize native level parameters right away
        try {
            int[] secureStarterGrid = new int[200]; 
            for (int i = 0; i < secureStarterGrid.length; i++) {
                secureStarterGrid[i] = 1; 
            }
            initNativeLevel(secureStarterGrid);
        } catch (Throwable nativeError) {
            throw new RuntimeException("Crash while transferring level matrix configuration data down into the C++ layer.", nativeError);
        }

        // 7. Synchronize thread surface drawing executions
        if (gameEngine instanceof SurfaceView) {
            SurfaceHolder holder = ((SurfaceView) gameEngine).getHolder();
            holder.addCallback(new SurfaceHolder.Callback() {
                @Override
                public void surfaceCreated(SurfaceHolder holder) {
                    isSurfaceReady = true;
                    if (gameEngine != null) {
                        try {
                            gameEngine.resume();
                        } catch (Throwable t) {
                            throw new RuntimeException("Crash within GameEngine.resume() thread start loop during surface setup!", t);
                        }
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
