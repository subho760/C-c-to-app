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
    public native int getNativeLives();
    public native void updateNativeGame();
    public native void handleNativeTouch(float x, float y);
    public native float getNativePlayerX();
    public native float getNativePlayerY();

    private GameView gameView;
    private Handler gameHandler = new Handler();
    private int loadingTicks = 0;

    static {
        System.loadLibrary("C-c-to-app"); 
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        
        gameView = new GameView(this);
        setContentView(gameView);

        gameHandler.postDelayed(new Runnable() {
            @Override
            public void run() {
                if (getNativeGameState() == 0) {
                    loadingTicks++;
                    if (loadingTicks > 100) { 
                        setNativeGameState(1); 
                    }
                } else {
                    updateNativeGame();
                }
                gameView.invalidate(); 
                gameHandler.postDelayed(this, 16);
            }
        }, 16);
    }

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

            canvas.drawColor(Color.parseColor("#000000"));

            if (state == 0) {
                paint.setColor(Color.WHITE);
                paint.setTextSize(60);
                canvas.drawText("Loading Engine...", 200, 800, paint);
                
            } else if (state == 1) {
                paint.setColor(Color.parseColor("#00F3FF")); 
                paint.setTextSize(80);
                canvas.drawText("NEON MAZE GAME", 150, 400, paint);

                paint.setColor(Color.WHITE);
                paint.setTextSize(50);
                canvas.drawText("TAP ANYWHERE TO PLAY", 200, 800, paint);
                
            } else if (state == 2) {
                paint.setColor(Color.parseColor("#39FF14")); 
                paint.setStrokeWidth(15);
                canvas.drawLine(100, 500, 900, 500, paint); 

                float px = getNativePlayerX();
                float py = getNativePlayerY();
                paint.setColor(Color.parseColor("#00F3FF")); 
                canvas.drawCircle(px, py, 30, paint);

                paint.setColor(Color.WHITE);
                paint.setTextSize(40);
                canvas.drawText("Lives: " + getNativeLives(), 50, 100, paint);
                
            } else if (state == 3) {
                paint.setColor(Color.RED);
                paint.setTextSize(80);
                canvas.drawText("GAME OVER", 250, 500, paint);
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
