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

public class MainActivity extends Activity {

    // Native JNI Methods
    public native int getNativeGameState();
    public native void setNativeGameState(int state);
    public native jint getNativeLives();
    public native void updateNativeGame();
    public native void handleNativeTouch(float x, float y);
    public native float getNativePlayerX();
    public native float getNativePlayerY();

    private GameView gameView;
    private Handler gameHandler = new Handler();
    private int loadingTicks = 0;

    static {
        // Ensure this matches the exact name defined in your CMakeLists.txt!
        System.loadLibrary("C-c-to-app"); 
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        
        // Create the game display dynamically to avoid layout XML configuration crashes
        gameView = new GameView(this);
        setContentView(gameView);

        // Simple 60FPS Game loop updates
        gameHandler.postDelayed(new Runnable() {
            @Override
            public void run() {
                if (getNativeGameState() == 0) {
                    loadingTicks++;
                    if (loadingTicks > 100) { // Simulate a 1.5 second loading screen
                        setNativeGameState(1); // Move to Main Menu
                    }
                } else {
                    updateNativeGame();
                }
                gameView.invalidate(); // Force screen redraw
                gameHandler.postDelayed(this, 16);
            }
        }, 16);
    }

    // High Performance custom drawing engine
    class GameView extends View {
        private Paint paint = new Paint();

        public GameView(Context context) {
            super(context);
            paint.setAntiAlias(true);
        }

        @Override
        protected void onDraw(Canvas canvas) {
            super.onDraw(canvas);
            int state = getNativeGameState();

            // 1. Dark Theme Background
            canvas.drawColor(Color.parseColor("#000000"));

            if (state == 0) {
                // LOADING SCREEN
                paint.setColor(Color.WHITE);
                paint.setTextSize(60);
                canvas.drawText("Loading Engine...", 200, 800, paint);
                
            } else if (state == 1) {
                // MAIN MENU SCREEN
                paint.setColor(Color.parseColor("#00F3FF")); // Neon Cyan Title
                paint.setTextSize(80);
                canvas.drawText("NEON MAZE GAME", 150, 400, paint);

                paint.setColor(Color.WHITE);
                paint.setTextSize(50);
                canvas.drawText("TAP ANYWHERE TO PLAY", 200, 800, paint);
                
            } else if (state == 2) {
                // GAMEPLAY MODE
                paint.setColor(Color.parseColor("#39FF14")); // Neon Green Track Paths
                paint.setStrokeWidth(15);
                canvas.drawLine(100, 500, 900, 500, paint); // Static track path example

                // Draw moving player node calculated by C++ layer
                float px = getNativePlayerX();
                float py = getNativePlayerY();
                paint.setColor(Color.parseColor("#00F3FF")); // Neon Cyan Player Orb
                canvas.drawCircle(px, py, 30, paint);

                // Show HUD Data
                paint.setColor(Color.WHITE);
                paint.setTextSize(40);
                canvas.drawText("Lives: " + getNativeLives(), 50, 100, paint);
                
            } else if (state == 3) {
                // GAME OVER SCREEN
                paint.setColor(Color.RED);
                paint.setTextSize(80);
                canvas.drawText("GAME OVER", 250, 500, paint);
                
                paint.setColor(Color.WHITE);
                paint.setTextSize(40);
                canvas.drawText("Tap to Retry (Ad Placement Stub)", 150, 800, paint);
            }
        }

        @Override
        public boolean onTouchEvent(MotionEvent event) {
            if (event.getAction() == MotionEvent.ACTION_DOWN || event.getAction() == MotionEvent.ACTION_MOVE) {
                handleNativeTouch(event.getX(), event.getY());
            }
            return true;
        }
    }
}
