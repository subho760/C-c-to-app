package com.night.backgroundchange;

import android.app.Activity;
import android.app.Dialog;
import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.RectF;
import android.graphics.Typeface; // Added missing import
import android.graphics.drawable.ColorDrawable; // Added missing import
import android.graphics.drawable.GradientDrawable;
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

    // Native JNI Game Engine Linkers
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

        // Core Game Loop
        gameHandler.postDelayed(new Runnable() {
            @Override
            public void run() {
                try {
                    int state = getNativeGameState();
                    if (state == 2) { 
                        updateNativeGame();
                        if (getNativeLives() <= 0 && !isDialogShowing) {
                            showExactAdDialog();
                        }
                    }
                    gameView.invalidate();
                    gameHandler.postDelayed(this, 16);
                } catch (Exception e) {
                    // Smooth continuation
                }
            }
        }, 16);
    }

    // Pop-up layout matching your original game box perfectly
    private void showExactAdDialog() {
        isDialogShowing = true;
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                adDialog = new Dialog(MainActivity.this);
                adDialog.requestWindowFeature(Window.FEATURE_NO_TITLE);
                
                // Outer Dialog Container
                LinearLayout layout = new LinearLayout(MainActivity.this);
                layout.setOrientation(LinearLayout.VERTICAL);
                layout.setPadding(60, 65, 60, 65);
                layout.setGravity(Gravity.CENTER_HORIZONTAL);
                
                GradientDrawable dialogBg = new GradientDrawable();
                dialogBg.setColor(Color.parseColor("#212124")); 
                dialogBg.setCornerRadius(24f); 
                layout.setBackground(dialogBg);

                // Title Header
                TextView title = new TextView(MainActivity.this);
                title.setText("Continue?");
                title.setTextColor(Color.WHITE);
                title.setTextSize(22);
                title.setTypeface(Typeface.create("sans-serif-medium", Typeface.NORMAL));
                title.setGravity(Gravity.CENTER);
                layout.addView(title);

                // Lives Indicator Section
                TextView hearts = new TextView(MainActivity.this);
                hearts.setText("❤️ ❤️ ❤️");
                hearts.setTextSize(26);
                hearts.setGravity(Gravity.CENTER);
                hearts.setPadding(0, 25, 0, 25);
                layout.addView(hearts);

                // Subtitle Instructions
                TextView message = new TextView(MainActivity.this);
                message.setText("Watch an ad to refill your lives\nand keep playing!");
                message.setTextColor(Color.parseColor("#8E8E93"));
                message.setTextSize(14);
                message.setGravity(Gravity.CENTER);
                message.setLineSpacing(4f, 1f);
                message.setPadding(0, 0, 0, 45);
                layout.addView(message);

                // Action Call Button
                Button adButton = new Button(MainActivity.this);
                adButton.setText("📺 ADD MORE LIVES");
                adButton.setTextColor(Color.WHITE);
                adButton.setTextSize(14);
                adButton.setTypeface(Typeface.DEFAULT_BOLD);
                
                GradientDrawable btnBg = new GradientDrawable();
                btnBg.setColor(Color.parseColor("#3F51B5")); 
                btnBg.setCornerRadius(8f);
                adButton.setBackground(btnBg);
                
                LinearLayout.LayoutParams btnParams = new LinearLayout.LayoutParams(
                        LinearLayout.LayoutParams.MATCH_PARENT, 130);
                adButton.setLayoutParams(btnParams);
                adButton.setOnClickListener(new View.OnClickListener() {
                    @Override
                    public void onClick(View v) {
                        resetNativeGame();
                        isDialogShowing = false;
                        adDialog.dismiss();
                    }
                });
                layout.addView(adButton);

                // Decline/Reset Option Button
                Button restartButton = new Button(MainActivity.this);
                restartButton.setText("RESTART");
                restartButton.setTextColor(Color.parseColor("#B0B0B5"));
                restartButton.setTextSize(13);
                restartButton.setTypeface(Typeface.DEFAULT_BOLD);
                restartButton.setBackgroundColor(Color.TRANSPARENT);
                
                LinearLayout.LayoutParams restartParams = new LinearLayout.LayoutParams(
                        LinearLayout.LayoutParams.WRAP_CONTENT, LinearLayout.LayoutParams.WRAP_CONTENT);
                restartParams.setMargins(0, 30, 0, 0);
                restartButton.setLayoutParams(restartParams);
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
                    lp.dimAmount = 0.65f; 
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
            
            // Sharp line vector paths for drawing original arrows natively
            arrowPath.moveTo(0, -18);
            arrowPath.lineTo(14, -2);
            arrowPath.lineTo(5, -2);
            arrowPath.lineTo(5, 18);
            arrowPath.lineTo(-5, 18);
            arrowPath.lineTo(-5, -2);
            arrowPath.lineTo(-14, -2);
            arrowPath.close();
        }

        @Override
        protected void onDraw(Canvas canvas) {
            super.onDraw(canvas);
            int state = getNativeGameState();
            
            // Slate canvas background color
            canvas.drawColor(Color.parseColor("#121316")); 

            if (state == 0) {
                setNativeGameState(1);
            } else if (state == 1) {
                setNativeGameState(2);
            } else {
                // Title UI Trackers
                paint.setColor(Color.WHITE);
                paint.setTextSize(55);
                paint.setTextAlign(Paint.Align.CENTER);
                paint.setTypeface(Typeface.create("sans-serif-light", Typeface.NORMAL));
                canvas.drawText("▲ rrows", getWidth() / 2f, 130, paint);
                
                paint.setColor(Color.parseColor("#5C6BC0"));
                paint.setTextSize(38);
                paint.setTypeface(Typeface.create("sans-serif", Typeface.NORMAL));
                canvas.drawText("Level " + getNativeLevel(), getWidth() / 2f, 195, paint);

                // Grid Matrix Layout
                int rows = 6;
                int cols = 4;
                float spacingX = getWidth() / (cols + 1f);
                float spacingY = (getHeight() - 400f) / (rows + 1f);

                for (int r = 1; r <= rows; r++) {
                    for (int c = 1; c <= cols; c++) {
                        float ax = c * spacingX;
                        float ay = 260 + r * spacingY;
                        
                        canvas.save();
                        canvas.translate(ax, ay);
                        
                        float rotation = (r % 2 == 0) ? (c * 90f) : (c * -90f);
                        canvas.rotate(rotation);
                        
                        paint.setStyle(Paint.Style.STROKE);
                        paint.setStrokeWidth(4.5f);
                        
                        if (getNativeLives() < 3 && r == 4 && c == 1) {
                            paint.setColor(Color.parseColor("#7F3A3A")); 
                        } else {
                            paint.setColor(Color.parseColor("#28292E")); 
                        }
                        
                        canvas.drawPath(arrowPath, paint);
                        canvas.restore();
                    }
                }

                // Player Node
                float px = getNativePlayerX();
                float py = getNativePlayerY();
                if (px == 0) { 
                    px = getWidth() / 2f; 
                    py = getHeight() - 180f; 
                }

                paint.setStyle(Paint.Style.FILL);
                paint.setColor(Color.parseColor("#3CD0E6")); 
                canvas.drawCircle(px, py, 28, paint);
            }
        }

        @Override
        public boolean onTouchEvent(MotionEvent event) {
            if (event.getAction() == MotionEvent.ACTION_DOWN || event.getAction() == MotionEvent.ACTION_MOVE) {
                if (getNativeGameState() == 2) {
                    handleNativeTouch(event.getX(), event.getY());
                }
            }
            return true;
        }
    }
}
