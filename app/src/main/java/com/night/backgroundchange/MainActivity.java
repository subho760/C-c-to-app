package com.night.backgroundchange;

import android.app.Activity;
import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.os.Bundle;
import android.os.Handler;
import android.view.MotionEvent;
import android.view.View;
import android.widget.TextView;

public class MainActivity extends Activity {

    // Native JNI Methods
    public native int getNativeGameState();
    public native void setNativeGameState(int state);
    public native int getNativeLives();
    public native void updateNativeGame();
    public native void handleNativeTouch(float x, float y);
    public native float getNativePlayerX();
    public native float getNativePlayerY();

    private GameView gameView;
    private Handler gameHandler = new Handler();
    private int loadingTicks = 0;
    private static String initError = null;

    static {
        // Smart loader loop to check all possible names your wrapper template might use
        String[] possibleLibraries = {"backgroundchange", "native-lib", "C-c-to-app", "main"};
        boolean loaded = false;
        StringBuilder log = new StringBuilder();

        for (String lib : possibleLibraries) {
            try {
                System.loadLibrary(lib);
                loaded = true;
                break; // Found it! Stop searching.
            } catch (UnsatisfiedLinkError e) {
                log.append(lib).append(" failed; ");
            }
        }

        if (!loaded) {
            initError = "Library Load Failed. Checked names: " + log.toString();
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        
        // If all library names failed to load, show the debug error screen
        if (initError != null) {
            TextView errorView = new TextView(this);
            errorView.setText(initError);
            errorView.setTextColor(Color.RED);
            errorView.setTextSize(16);
            errorView.setPadding(30, 50, 30, 30);
            setContentView(errorView);
            return;
        }

        try {
            gameView = new GameView(this);
            setContentView(gameView);

            // 60 FPS Engine Game Loop
            gameHandler.postDelayed(new Runnable() {
                @Override
                public void run() {
                    try {
                        if (getNativeGameState() == 0) {
                            loadingTicks++;
                            if (loadingTicks > 60) { // Transition to menu after ~1 second
                                setNativeGameState(1); 
                            }
                        } else {
                            updateNativeGame();
                        }
                        gameView.invalidate(); // Redraw the screen canvas
                        gameHandler.postDelayed(this, 16);
                    } catch (Exception e) {
                        // Prevent thread crash loop
                    }
                }
            }, 16);
        } catch (Exception e) {
            TextView errorView = new TextView(this);
            errorView.setText("UI Init Error: " + e.getMessage());
            setContentView(errorView);
        }
    }

    // Canvas graphic drawing engine (No OpenGL crash)
    class GameView extends View {
        private Paint paint = new Paint();

        public GameView(Context context) {
            super(context);
            paint.setAntiAlias(true);
        }

        @Override
        protected void onDraw(Canvas canvas) {
            super.onDraw(canvas);
            try {
                int state = getNativeGameState();
                
                // Pure black canvas layout
                canvas.drawColor(Color.parseColor("#000000"));

                if (state == 0) {
                    // LOADING SCREEN LAYER
                    paint.setColor(Color.WHITE);
                    paint.setTextSize(60);
                    paint.setTextAlign(Paint.Align.CENTER);
                    canvas.drawText("LOADING ENGINE...", getWidth() / 2f, getHeight() / 2f, paint);
                    
                } else if (state == 1) {
                    // MAIN MENU SCREEN LAYER
                    paint.setColor(Color.parseColor("#00F3FF")); // Neon Cyan
                    paint.setTextSize(80);
                    paint.setTextAlign(Paint.Align.CENTER);
                    canvas.drawText("NEON MAZE GAME", getWidth() / 2f, getHeight() / 3f, paint);

                    paint.setColor(Color.WHITE);
                    paint.setTextSize(45);
                    canvas.drawText("TAP ANYWHERE TO PLAY", getWidth() / 2f, getHeight() / 1.5f, paint);
                    
                } else if (state == 2) {
                    // LIVE GAMEPLAY CANVAS LAYER
                    paint.setColor(Color.parseColor("#39FF14")); // Neon Green Path Track
                    paint.setStrokeWidth(20);
                    paint.setStyle(Paint.Style.STROKE);
                    // Simple demonstration path lines
                    canvas.drawLine(100, getHeight() / 2f, getWidth() - 100, getHeight() / 2f, paint);

                    // Draw moving player position requested from native memory
                    float px = getNativePlayerX();
                    float py = getNativePlayerY();
                    paint.setStyle(Paint.Style.FILL);
                    paint.setColor(Color.parseColor("#00F3FF")); // Neon Cyan Moving Player Dot
                    canvas.drawCircle(px, py, 35, paint);

                    // HUD Display Text
                    paint.setColor(Color.WHITE);
                    paint.setTextSize(45);
                    paint.setTextAlign(Paint.Align.LEFT);
                    canvas.drawText("LIVES: " + getNativeLives(), 50, 100, paint);
                    
                } else if (state == 3) {
                    // GAME OVER SCREEN LAYER
                    paint.setColor(Color.RED);
                    paint.setTextSize(90);
                    paint.setTextAlign(Paint.Align.CENTER);
                    canvas.drawText("GAME OVER", getWidth() / 2f, getHeight() / 2f, paint);
                    
                    paint.setColor(Color.WHITE);
                    paint.setTextSize(40);
                    canvas.drawText("Tap Screen to Restart", getWidth() / 2f, getHeight() / 1.5f, paint);
                }
            } catch (Exception e) {
                // Keep canvas safe from missing values during transitions
            }
        }

        @Override
        public boolean onTouchEvent(MotionEvent event) {
            try {
                if (event.getAction() == MotionEvent.ACTION_DOWN || event.getAction() == MotionEvent.ACTION_MOVE) {
                    handleNativeTouch(event.getX(), event.getY());
                }
            } catch (Exception e) {
                // Safely catch anomalies
            }
            return true;
        }
    }
}
