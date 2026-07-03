package com.night.backgroundchange;

import android.app.Activity;
import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.os.Bundle;
import android.view.MotionEvent;
import android.view.View;
import android.widget.Button;
import android.widget.RelativeLayout;
import android.widget.TextView;

public class MainActivity extends Activity {

    static { System.loadLibrary("game_logic"); }

    private GameView gameView;
    private RelativeLayout menuLayout;
    private TextView loadingText;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        
        // Setup Root Layout
        RelativeLayout root = new RelativeLayout(this);
        root.setBackgroundColor(Color.BLACK);

        // 1. The Game Canvas
        gameView = new GameView(this);
        root.addView(gameView);

        // 2. Main Menu Overlay
        menuLayout = new RelativeLayout(this);
        Button btnPlay = new Button(this);
        btnPlay.setText("PLAY");
        btnPlay.setOnClickListener(v -> {
            nativeOnPlayClicked();
            menuLayout.setVisibility(View.GONE);
        });
        menuLayout.addView(btnPlay);
        root.addView(menuLayout);

        // 3. Loading Text
        loadingText = new TextView(this);
        loadingText.setText("Loading...");
        loadingText.setTextColor(Color.CYAN);
        root.addView(loadingText);

        setContentView(root);
        nativeInit();
    }

    // Custom View to draw what C++ calculates
    class GameView extends View {
        Paint neonPaint = new Paint();
        Paint dimmedPaint = new Paint();

        public GameView(Context context) {
            super(context);
            neonPaint.setColor(Color.parseColor("#00F3FF")); // Neon Cyan
            neonPaint.setStrokeWidth(8f);
            neonPaint.setStyle(Paint.Style.STROKE);
            
            dimmedPaint.setColor(Color.parseColor("#333333")); // Dimmed Grey
            dimmedPaint.setStrokeWidth(5f);
        }

        @Override
        protected void onDraw(Canvas canvas) {
            int state = nativeUpdate(0.016f); // Approx 60fps delta

            if (state == 0) { // LOADING
                loadingText.setVisibility(View.VISIBLE);
            } else {
                loadingText.setVisibility(View.GONE);
                
                // Draw Arrows from C++ Data
                int count = nativeGetArrowCount();
                for (int i = 0; i < count; i++) {
                    float[] data = nativeGetArrowData(i);
                    // data: [x1, y1, x2, y2, dir, isActive]
                    Paint p = (data[5] == 1.0f) ? neonPaint : dimmedPaint;
                    canvas.drawLine(data[0], data[1], data[2], data[3], p);
                    // (Logic to draw arrow head based on dir would go here)
                }
            }
            invalidate(); // Force redraw
        }

        @Override
        public boolean onTouchEvent(MotionEvent event) {
            if (event.getAction() == MotionEvent.ACTION_DOWN) {
                nativeTouch(event.getX(), event.getY());
            }
            return true;
        }
    }

    // Native Definitions
    public native void nativeInit();
    public native int nativeUpdate(float delta);
    public native int nativeGetArrowCount();
    public native float[] nativeGetArrowData(int index);
    public native void nativeOnPlayClicked();
    public native void nativeTouch(float x, float y);
}
