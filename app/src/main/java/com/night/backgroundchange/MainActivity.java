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
        try {
            // Hardcoded explicitly to prevent Gradle packaging artifacts splitting
            System.loadLibrary("C-c-to-app");
        } catch (UnsatisfiedLinkError e) {
            initError = "Library Load Failed: " + e.getMessage();
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        
        // Show runtime errors explicitly on screen if linking fails
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

            // 60 FPS Game Loop
            gameHandler.postDelayed(new Runnable() {
                @Override
                public void run() {
                    try {
                        if (getNativeGameState() == 0) {
                            loadingTicks++;
                            if (loadingTicks > 60) { 
                                setNativeGameState(1); 
                            }
                        } else {
                            updateNativeGame();
                        }
                        gameView.invalidate(); 
                        gameHandler.postDelayed(this, 16);
                    } catch (Exception e) {
                        // Prevent thread lock
                    }
                }
            }, 16);
        } catch (Exception e) {
            TextView errorView = new TextView(this);
            errorView.setText("UI Init Error: " + e.getMessage());
            setContentView(errorView);
        }
    }

    // Canvas rendering engine layout
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
                canvas.drawColor(Color.parseColor("#000000")); // Solid Black Canvas

                if (state == 0) {
                    paint.setColor(Color.WHITE);
                    paint.setTextSize(60);
                    paint.setTextAlign(Paint.Align.CENTER);
                    canvas.drawText("LOADING ENGINE...", getWidth() / 2f, getHeight() / 2f, paint);
                    
                } else if (state == 1) {
                    paint.setColor(Color.parseColor("#00F3FF")); // Neon Cyan
                    paint.setTextSize(80);
                    paint.setTextAlign(Paint.Align.CENTER);
                    canvas.drawText("NEON MAZE GAME", getWidth() / 2f, getHeight() / 3f, paint);

                    paint.setColor(Color.WHITE);
                    paint.setTextSize(45);
                    canvas.drawText("TAP ANYWHERE TO PLAY", getWidth() / 2f, getHeight() / 1.5f, paint);
                    
                } else if (state == 2) {
                    paint.setColor(Color.parseColor("#39FF14")); // Neon Green Track Path
                    paint.setStrokeWidth(20);
                    paint.setStyle(Paint.Style.STROKE);
                    canvas.drawLine(100, getHeight() / 2f, getWidth() - 100, getHeight() / 2f, paint);

                    float px = getNativePlayerX();
                    float py = getNativePlayerY();
                    paint.setStyle(Paint.Style.FILL);
                    paint.setColor(Color.parseColor("#00F3FF")); // Neon Player Dot
                    canvas.drawCircle(px, py, 35, paint);

                    paint.setColor(Color.WHITE);
                    paint.setTextSize(45);
                    paint.setTextAlign(Paint.Align.LEFT);
                    canvas.drawText("LIVES: " + getNativeLives(), 50, 100, paint);
                    
                } else if (state == 3) {
                    paint.setColor(Color.RED);
                    paint.setTextSize(90);
                    paint.setTextAlign(Paint.Align.CENTER);
                    canvas.drawText("GAME OVER", getWidth() / 2f, getHeight() / 2f, paint);
                    
                    paint.setColor(Color.WHITE);
                    paint.setTextSize(40);
                    canvas.drawText("Tap Screen to Restart", getWidth() / 2f, getHeight() / 1.5f, paint);
                }
            } catch (Exception e) {
                // Safeguard graphics
            }
        }

        @Override
        public boolean onTouchEvent(MotionEvent event) {
            try {
                if (event.getAction() == MotionEvent.ACTION_DOWN || event.getAction() == MotionEvent.ACTION_MOVE) {
                    handleNativeTouch(event.getX(), event.getY());
                }
            } catch (Exception e) {
                // Ignore ghost touches
            }
            return true;
        }
    }
}
