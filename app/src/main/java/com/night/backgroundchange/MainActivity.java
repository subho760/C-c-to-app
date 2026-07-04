package com.night.backgroundchange;

import android.app.Activity;
import android.app.Dialog;
import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.drawable.ColorDrawable;
import android.os.Bundle;
import android.os.Handler;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.TextView;

public class MainActivity extends Activity {

    // Native JNI Game Engine Layer
    public native int getNativeGameState();
    public native void setNativeGameState(int state);
    public native int getNativeLives();
    public native void resetNativeGame();
    public native void updateNativeGame();
    public native void handleNativeTouch(float x, float y);
    public native float getNativePlayerX();
    public native float getNativePlayerY();
    public native int getNativeLevel();

    private GameView gameView;
    private Handler gameHandler = new Handler();
    private Dialog adDialog;
    private boolean isDialogShowing = false;
    private static String initError = null;

    static {
        try {
            System.loadLibrary("C-c-to-app");
        } catch (UnsatisfiedLinkError e) {
            initError = "Library Load Failed: " + e.getMessage();
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        
        if (initError != null) {
            TextView errorView = new TextView(this);
            errorView.setText(initError);
            errorView.setTextColor(Color.RED);
            errorView.setTextSize(16);
            setContentView(errorView);
            return;
        }

        gameView = new GameView(this);
        setContentView(gameView);

        // Core 60 FPS Engine Loop
        gameHandler.postDelayed(new Runnable() {
            @Override
            public void run() {
                try {
                    int state = getNativeGameState();
                    if (state == 2) { // Playing
                        updateNativeGame();
                        if (getNativeLives() <= 0 && !isDialogShowing) {
                            showAdDialog();
                        }
                    }
                    gameView.invalidate();
                    gameHandler.postDelayed(this, 16);
                } catch (Exception e) {
                    // Prevent context frame drops
                }
            }
        }, 16);
    }

    // 🎬 Shows the "Watch an ad to refill your lives" Popup Dialog from your video
    private void showAdDialog() {
        isDialogShowing = true;
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                adDialog = new Dialog(MainActivity.this);
                adDialog.requestWindowFeature(Window.FEATURE_NO_TITLE);
                
                // Custom layout matching the video UI
                LinearLayout layout = new LinearLayout(MainActivity.this);
                layout.setOrientation(LinearLayout.VERTICAL);
                layout.setBackgroundColor(Color.parseColor("#1E1F22"));
                layout.setPadding(50, 50, 50, 50);
                layout.setGravity(Gravity.CENTER);

                TextView title = new TextView(MainActivity.this);
                title.setText("Continue?");
                title.setTextColor(Color.WHITE);
                title.setTextSize(24);
                title.setGravity(Gravity.CENTER);
                layout.addView(title);

                // Heart row simulation
                TextView hearts = new TextView(MainActivity.this);
                hearts.setText("❤️ ❤️ ❤️");
                hearts.setTextSize(30);
                hearts.setGravity(Gravity.CENTER);
                hearts.setPadding(0, 20, 0, 20);
                layout.addView(hearts);

                TextView message = new TextView(MainActivity.this);
                message.setText("Watch an ad to refill your lives\nand keep playing!");
                message.setTextColor(Color.GRAY);
                message.setTextSize(14);
                message.setGravity(Gravity.CENTER);
                message.setPadding(0, 0, 0, 40);
                layout.addView(message);

                Button adButton = new Button(MainActivity.this);
                adButton.setText("📺 Add More Lives");
                adButton.setBackgroundColor(Color.parseColor("#3F51B5"));
                adButton.setTextColor(Color.WHITE);
                adButton.setOnClickListener(new View.OnClickListener() {
                    @Override
                    public void onClick(View v) {
                        resetNativeGame();
                        isDialogShowing = false;
                        adDialog.dismiss();
                    }
                });
                layout.addView(adButton);

                Button restartButton = new Button(MainActivity.this);
                restartButton.setText("Restart");
                restartButton.setBackgroundColor(Color.TRANSPARENT);
                restartButton.setTextColor(Color.WHITE);
                restartButton.setPadding(0, 30, 0, 0);
                restartButton.setOnClickListener(new View.OnClickListener() {
                    @Override
                    public void onClick(View v) {
                        resetNativeGame();
                        isDialogShowing = false;
                        adDialog.dismiss();
                    }
                });
                layout.addView(restartButton);

                adDialog.setContentView(layout);
                adDialog.setCancelable(false);
                
                Window window = adDialog.getWindow();
                if (window != null) {
                    window.setBackgroundDrawable(new ColorDrawable(Color.TRANSPARENT));
                    WindowManager.LayoutParams lp = window.getAttributes();
                    lp.dimAmount = 0.7f;
                    window.setAttributes(lp);
                }
                adDialog.show();
            }
        });
    }

    class GameView extends View {
        private Paint paint = new Paint();
        private Path arrowPath = new Path();

        public GameView(Context context) {
            super(context);
            paint.setAntiAlias(true);
            
            // Define standard vector path shape for rendering maze arrows
            arrowPath.moveTo(0, -15);
            arrowPath.lineTo(15, 5);
            arrowPath.lineTo(5, 5);
            arrowPath.lineTo(5, 20);
            arrowPath.lineTo(-5, 20);
            arrowPath.lineTo(-5, 5);
            arrowPath.lineTo(-15, 5);
            arrowPath.close();
        }

        @Override
        protected void onDraw(Canvas canvas) {
            super.onDraw(canvas);
            int state = getNativeGameState();
            
            // Deep sleek background color matching your gameplay video
            canvas.drawColor(Color.parseColor("#16171B"));

            if (state == 0) { // Splash Screen
                paint.setColor(Color.WHITE);
                paint.setTextSize(50);
                paint.setTextAlign(Paint.Align.CENTER);
                canvas.drawText("LOADING...", getWidth() / 2f, getHeight() / 2f, paint);
                setNativeGameState(1);
                
            } else if (state == 1) { // Main Title Screen
                paint.setColor(Color.WHITE);
                paint.setTextSize(60);
                paint.setTextAlign(Paint.Align.CENTER);
                canvas.drawText("▲ rrows", getWidth() / 2f, getHeight() / 3f, paint);
                
                paint.setColor(Color.parseColor("#3F51B5"));
                canvas.drawRect(getWidth()/4f, getHeight()/2f, getWidth()*3/4f, getHeight()/2f + 100, paint);
                paint.setColor(Color.WHITE);
                paint.setTextSize(40);
                canvas.drawText("Play", getWidth() / 2f, getHeight() / 2f + 65, paint);
                
            } else { // Active Maze Mode (State 2)
                // Draw Header Title Info
                paint.setColor(Color.WHITE);
                paint.setTextSize(45);
                paint.setTextAlign(Paint.Align.CENTER);
                canvas.drawText("▲rrows", getWidth() / 2f, 100, paint);
                
                paint.setColor(Color.parseColor("#5C6BC0"));
                paint.setTextSize(35);
                canvas.drawText("Level " + getNativeLevel(), getWidth() / 2f, 160, paint);

                // Build & Draw the Grid Arrow Maze System
                int rows = 6;
                int cols = 4;
                float spacingX = getWidth() / (cols + 1f);
                float spacingY = (getHeight() - 300f) / (rows + 1f);

                for (int r = 1; r <= rows; r++) {
                    for (int c = 1; c <= cols; c++) {
                        float ax = c * spacingX;
                        float ay = 200 + r * spacingY;
                        
                        canvas.save();
                        canvas.translate(ax, ay);
                        
                        // Rotational matrix pattern simulation
                        float rotation = (r % 2 == 0) ? (c * 90f) : (c * -90f);
                        canvas.rotate(rotation);
                        
                        paint.setStyle(Paint.Style.STROKE);
                        paint.setStrokeWidth(4);
                        // Danger highlight track color toggle
                        if (getNativeLives() < 3 && r == 4) {
                            paint.setColor(Color.parseColor("#E57373")); // Soft Crimson Alert
                        } else {
                            paint.setColor(Color.parseColor("#3A3C45"));
                        }
                        
                        canvas.drawPath(arrowPath, paint);
                        canvas.restore();
                    }
                }

                // Render the Player Dot element
                float px = getNativePlayerX();
                float py = getNativePlayerY();
                if (px == 0) { px = getWidth() / 2f; py = getHeight() - 200f; }

                paint.setStyle(Paint.Style.FILL);
                paint.setColor(Color.parseColor("#4DD0E1")); // Electric Cyan Player Node
                canvas.drawCircle(px, py, 30, paint);
            }
        }

        @Override
        public boolean onTouchEvent(MotionEvent event) {
            if (event.getAction() == MotionEvent.ACTION_DOWN) {
                if (getNativeGameState() == 1) {
                    setNativeGameState(2); // Start gameplay direct
                } else if (getNativeGameState() == 2) {
                    handleNativeTouch(event.getX(), event.getY());
                }
            }
            return true;
        }
    }
}
