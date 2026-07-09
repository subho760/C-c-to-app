package com.night.backgroundchange;

import android.content.Context;
import android.graphics.*;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;

public class GameEngine extends SurfaceView implements Runnable {
    private Thread gameThread;
    private boolean isPlaying;
    private SurfaceHolder surfaceHolder;
    private Canvas canvas;
    private Paint paint;
    private Bitmap arrowBitmap, glowBitmap;
    
    private List<ArrowSprite> arrows = new ArrayList<>();
    private List<Particle> particles = new ArrayList<>();
    private int currentLevel = 1;
    private boolean isDarkMode = true;
    private MainActivity activity;

    public GameEngine(Context context, MainActivity activity) {
        super(context);
        this.activity = activity;
        surfaceHolder = getHolder();
        paint = new Paint();
        loadBitmaps();
        loadLevel(1);
    }

    private void loadBitmaps() {
        arrowBitmap = BitmapFactory.decodeResource(getResources(), R.drawable.arrow);
        glowBitmap = BitmapFactory.decodeResource(getResources(), R.drawable.glow);
    }

    public void loadLevel(int level) {
        currentLevel = level;
        arrows.clear();
        List<LevelManager.ArrowDef> defs = LevelManager.getLevel(level);
        int[] nativeData = new int[defs.size() * 4];
        for (int i = 0; i < defs.size(); i++) {
            LevelManager.ArrowDef d = defs.get(i);
            arrows.add(new ArrowSprite(d.id, d.x, d.y, d.dir));
            nativeData[i*4] = d.id; nativeData[i*4+1] = d.x;
            nativeData[i*4+2] = d.y; nativeData[i*4+3] = d.dir;
        }
        activity.initNativeLevel(nativeData);
    }

    @Override
    public void run() {
        while (isPlaying) {
            update();
            draw();
            control();
        }
    }

    private void update() {
        Iterator<ArrowSprite> it = arrows.iterator();
        while (it.hasNext()) {
            ArrowSprite a = it.next();
            a.update();
            if (a.isOffScreen()) {
                activity.removeNativeArrow(a.id);
                it.remove();
                if (arrows.isEmpty()) activity.onLevelComplete();
            }
        }
        
        Iterator<Particle> pIt = particles.iterator();
        while (pIt.hasNext()) {
            if (pIt.next().update()) pIt.remove();
        }
    }

    private void draw() {
        if (surfaceHolder.getSurface().isValid()) {
            canvas = surfaceHolder.lockCanvas();
            canvas.drawColor(isDarkMode ? Color.BLACK : Color.WHITE);
            
            // Draw Arrows
            paint.setColorFilter(new PorterDuffColorFilter(
                isDarkMode ? Color.WHITE : Color.BLACK, PorterDuff.Mode.SRC_IN));
            
            for (ArrowSprite a : arrows) {
                Matrix matrix = new Matrix();
                matrix.postRotate(a.dir * 90, arrowBitmap.getWidth()/2f, arrowBitmap.getHeight()/2f);
                matrix.postTranslate(a.currentX, a.currentY);
                matrix.postScale(a.scale, a.scale, a.currentX + arrowBitmap.getWidth()/2f, a.currentY + arrowBitmap.getHeight()/2f);
                canvas.drawBitmap(arrowBitmap, matrix, paint);
            }

            // Draw Particles
            paint.setColorFilter(null);
            for (Particle p : particles) {
                paint.setAlpha((int)(p.alpha * 255));
                canvas.drawBitmap(glowBitmap, p.x, p.y, paint);
            }

            surfaceHolder.unlockCanvasAndPost(canvas);
        }
    }

    private void control() {
        try { Thread.sleep(16); } catch (InterruptedException e) {}
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        if (event.getAction() == MotionEvent.ACTION_DOWN) {
            for (ArrowSprite a : arrows) {
                if (a.getBounds().contains((int)event.getX(), (int)event.getY())) {
                    if (activity.canArrowMove(a.id)) {
                        a.startFlying();
                        spawnParticles(a.currentX, a.currentY);
                        activity.playSound(false);
                    } else {
                        a.shake();
                    }
                    break;
                }
            }
        }
        return true;
    }

    private void spawnParticles(float x, float y) {
        for(int i=0; i<10; i++) particles.add(new Particle(x, y));
    }

    public void resume() { isPlaying = true; gameThread = new Thread(this); gameThread.start(); }
    public void pause() { isPlaying = false; try { gameThread.join(); } catch (InterruptedException e) {} }
    public void setDarkMode(boolean dark) { this.isDarkMode = dark; }
}
