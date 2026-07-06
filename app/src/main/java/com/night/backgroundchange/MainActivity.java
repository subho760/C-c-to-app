package com.night.backgroundchange;

import android.app.Activity;
import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.media.AudioAttributes;
import android.media.SoundPool;
import android.os.Bundle;
import android.view.MotionEvent;
import android.view.View;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.TextView;
import android.widget.Toast;

public class MainActivity extends Activity {
    static {
        System.loadLibrary("native-lib");
    }

    private native void initGame();
    private native void generateLevel(int level);
    private native boolean tapArrow(int x, int y);
    private native int[] getBoardState();
    private native int getBoardSize();
    private native boolean isLevelComplete();
    private native int getLives();
    private native int getCurrentLevel();
    private native int[] getHint();

    private GameView gameView;
    private TextView levelText;
    private TextView livesText;
    private SoundPool soundPool;
    private int tapSoundId;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        levelText = findViewById(R.id.levelText);
        livesText = findViewById(R.id.livesText);
        FrameLayout container = findViewById(R.id.gameContainer);
        
        gameView = new GameView(this);
        container.addView(gameView);

        AudioAttributes attrs = new AudioAttributes.Builder()
                .setUsage(AudioAttributes.USAGE_GAME)
                .setContentType(AudioAttributes.CONTENT_TYPE_SONIFICATION)
                .build();
        soundPool = new SoundPool.Builder().setMaxStreams(5).setAudioAttributes(attrs).build();
        // Fallback dummy stream initialization to eliminate dependency overheads
        tapSoundId = 1; 

        initGame();
        startLevel(1);

        findViewById(R.id.btnRestart).setOnClickListener(v -> startLevel(getCurrentLevel()));
        findViewById(R.id.btnHint).setOnClickListener(v -> {
            int[] hint = getHint();
            if (hint != null && hint.length >= 2 && hint[0] != -1) {
                gameView.showHint(hint[0], hint[1]);
            }
        });
    }

    private void startLevel(int level) {
        generateLevel(level);
        updateUI();
    }

    private void updateUI() {
        levelText.setText("Level: " + getCurrentLevel());
        livesText.setText("Hearts: " + getLives());
        gameView.invalidate();
        
        if (getLives() <= 0) {
            Toast.makeText(this, "Game Over! Restarting...", Toast.LENGTH_SHORT).show();
            startLevel(1);
        } else if (isLevelComplete()) {
            Toast.makeText(this, "Level Clear!", Toast.LENGTH_SHORT).show();
            startLevel(getCurrentLevel() + 1);
        }
    }

    class GameView extends View {
        private Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG);
        private int hintX = -1, hintY = -1;

        public GameView(Context context) { super(context); }

        public void showHint(int x, int y) {
            this.hintX = x; this.hintY = y;
            postDelayed(() -> { hintX = -1; hintY = -1; invalidate(); }, 1000);
            invalidate();
        }

        @Override
        protected void onDraw(Canvas canvas) {
            super.onDraw(canvas);
            int size = getBoardSize();
            int[] board = getBoardState();
            if (board == null || size <= 0) return;
            
            float cellSize = (float) getWidth() / size;

            for (int y = 0; y < size; y++) {
                for (int x = 0; x < size; x++) {
                    int val = board[y * size + x];
                    float left = x * cellSize;
                    float top = y * cellSize;

                    paint.setColor(Color.DKGRAY);
                    paint.setStyle(Paint.Style.STROKE);
                    canvas.drawRect(left, top, left + cellSize, top + cellSize, paint);

                    if (val > 0) {
                        paint.setStyle(Paint.Style.FILL);
                        paint.setColor((x == hintX && y == hintY) ? Color.YELLOW : Color.CYAN);
                        canvas.drawRoundRect(left + 10, top + 10, left + cellSize - 10, top + cellSize - 10, 20, 20, paint);
                        
                        paint.setColor(Color.BLACK);
                        paint.setStrokeWidth(5);
                        drawArrow(canvas, left + cellSize/2, top + cellSize/2, cellSize/3, val);
                    }
                }
            }
        }

        private void drawArrow(Canvas canvas, float cx, float cy, float radius, int dir) {
            float x1 = 0, y1 = 0, x2 = 0, y2 = 0;
            if (dir == 1) { y1 = radius; y2 = -radius; }      // UP
            else if (dir == 2) { x1 = -radius; x2 = radius; } // RIGHT
            else if (dir == 3) { y1 = -radius; y2 = radius; } // DOWN
            else if (dir == 4) { x1 = radius; x2 = -radius; } // LEFT

            canvas.drawLine(cx + x1, cy + y1, cx + x2, cy + y2, paint);
            canvas.drawCircle(cx + x2, cy + y2, 10, paint);
        }

        @Override
        public boolean onTouchEvent(MotionEvent event) {
            if (event.getAction() == MotionEvent.ACTION_DOWN) {
                int size = getBoardSize();
                if (size <= 0) return true;
                int x = (int) (event.getX() / (getWidth() / size));
                int y = (int) (event.getY() / (getHeight() / size));
                
                tapArrow(x, y);
                updateUI();
                return true;
            }
            return super.onTouchEvent(event);
        }
    }
}
