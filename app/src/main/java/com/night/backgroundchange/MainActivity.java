package com.night.backgroundchange;

import android.app.Activity;
import android.content.Context;
import android.graphics.*;
import android.os.Bundle;
import android.view.*;
import android.widget.*;

public class MainActivity extends Activity {
    static { System.loadLibrary("game_logic"); }

    private GameView gameView;
    private RelativeLayout uiContainer;
    private Button btnPlay;
    private TextView txtLevel;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // 1. Root Layout
        RelativeLayout root = new RelativeLayout(this);
        root.setBackgroundColor(Color.BLACK);

        // 2. Custom Canvas View
        gameView = new GameView(this);
        root.addView(gameView);

        // 3. Simple UI Overlay
        uiContainer = new RelativeLayout(this);
        btnPlay = new Button(this);
        btnPlay.setText("PLAY");
        btnPlay.setBackgroundColor(Color.parseColor("#00F3FF"));
        
        RelativeLayout.LayoutParams params = new RelativeLayout.LayoutParams(400, 150);
        params.addRule(RelativeLayout.CENTER_IN_PARENT);
        uiContainer.addView(btnPlay, params);

        txtLevel = new TextView(this);
        txtLevel.setText("Level 92");
        txtLevel.setTextColor(Color.WHITE);
        txtLevel.setTextSize(24);
        uiContainer.addView(txtLevel);

        root.addView(uiContainer);
        setContentView(root);

        btnPlay.setOnClickListener(v -> {
            nativeOnPlay();
            uiContainer.setVisibility(View.GONE);
        });

        nativeInit();
    }

    // Called from C++ JNI when lives reach 0
    public void onTriggerGameOverAd() {
        runOnUiThread(() -> {
            Toast.makeText(this, "Game Over! Rewarded Ad Loading...", Toast.LENGTH_LONG).show();
            // Stub for AdMob: rewardedAd.show(..., reward -> { nativeAddLives(); });
            // For now, auto-revive:
            nativeAddLives();
            uiContainer.setVisibility(View.GONE);
        });
    }

    // --- Custom Canvas Implementation ---
    class GameView extends View {
        private Paint neonPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
        private Paint dimPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
        private long lastTime = System.currentTimeMillis();

        public GameView(Context context) {
            super(context);
            neonPaint.setColor(Color.parseColor("#00F3FF")); // Neon Cyan
            neonPaint.setStyle(Paint.Style.STROKE);
            neonPaint.setStrokeWidth(12);
            // Simulate Neon Glow
            neonPaint.setShadowLayer(20, 0, 0, Color.parseColor("#00F3FF"));
            
            dimPaint.setColor(Color.parseColor("#333333")); // Dark Grey
            dimPaint.setStyle(Paint.Style.STROKE);
            dimPaint.setStrokeWidth(8);
            
            setLayerType(LAYER_TYPE_SOFTWARE, null); // Required for shadow effect
        }

        @Override
        protected void onDraw(Canvas canvas) {
            long now = System.currentTimeMillis();
            float dt = (now - lastTime) / 1000.0f;
            lastTime = now;

            int state = nativeUpdate(dt);

            if (state == 0) { // LOADING
                Paint textPaint = new Paint();
                textPaint.setColor(Color.WHITE);
                textPaint.setTextSize(60);
                canvas.drawText("Loading...", getWidth()/3f, getHeight()/2f, textPaint);
            } else if (state >= 2) { // PLAYING or GAMEOVER
                drawArrows(canvas);
            }

            invalidate(); // Loop the animation
        }

        private void drawArrows(Canvas canvas) {
            int count = nativeGetArrowCount();
            for (int i = 0; i < count; i++) {
                float[] data = nativeGetArrowData(i); // [x, y, dir, isPath, isActive]
                float x = data[0] * getWidth();
                float y = data[1] * getHeight();
                Paint p = (data[4] == 1.0f) ? neonPaint : dimPaint;

                // Draw a simple arrow shape
                canvas.drawCircle(x, y, 15, p);
                if (data[2] == 0) canvas.drawLine(x, y, x, y - 40, p); // Up
                if (data[2] == 1) canvas.drawLine(x, y, x + 40, y, p); // Right
                if (data[2] == 2) canvas.drawLine(x, y, x, y + 40, p); // Down
                if (data[2] == 3) canvas.drawLine(x, y, x - 40, y, p); // Left
            }
        }

        @Override
        public boolean onTouchEvent(MotionEvent event) {
            if (event.getAction() == MotionEvent.ACTION_DOWN) {
                nativeHandleTouch(event.getX() / getWidth(), event.getY() / getHeight());
            }
            return true;
        }
    }

    // --- Native JNI Interface ---
    public native void nativeInit();
    public native int nativeUpdate(float dt);
    public native int nativeGetArrowCount();
    public native float[] nativeGetArrowData(int index);
    public native void nativeHandleTouch(float tx, float ty);
    public native void nativeOnPlay();
    public native void nativeAddLives();
}
