package com.night.backgroundchange;

import android.app.Activity;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.view.MotionEvent;
import android.view.View;
import android.widget.Button;
import android.widget.RelativeLayout;
import android.widget.TextView;
import android.widget.Toast;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

public class MainActivity extends Activity implements GLSurfaceView.Renderer {

    static {
        System.loadLibrary("game_logic");
    }

    private GLSurfaceView glSurfaceView;
    private RelativeLayout uiOverlay;
    private Button btnPlay;
    private TextView txtLevel;
    private View loadingScreen;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        // 1. Setup SurfaceView for C++ Rendering
        glSurfaceView = new GLSurfaceView(this);
        glSurfaceView.setEGLContextClientVersion(2);
        glSurfaceView.setRenderer(this);
        
        RelativeLayout root = findViewById(R.id.main_layout);
        root.addView(glSurfaceView, 0);

        // 2. UI Elements (Matching Feature Removals)
        uiOverlay = findViewById(R.id.ui_overlay);
        btnPlay = findViewById(R.id.btnPlay);
        txtLevel = findViewById(R.id.txtLevel);
        loadingScreen = findViewById(R.id.loading_screen);

        btnPlay.setOnClickListener(v -> {
            nativeOnPlayClicked();
            uiOverlay.setVisibility(View.GONE);
        });

        nativeInitAssetManager(getAssets());
    }

    // --- JNI Callbacks from C++ ---

    // Triggered when hearts = 0 at 0:29 mark
    public void triggerRewardedAd() {
        runOnUiThread(() -> {
            Toast.makeText(this, "Ad Placeholder: Rewarded Video Loading...", Toast.LENGTH_SHORT).show();
            // AdMob logic here:
            // if (rewardedAd.isLoaded()) { rewardedAd.show(activity, rewardItem -> { nativeGrantLives(); }); }
            
            // Mocking success for demo:
            nativeGrantLives();
        });
    }

    public void onGameLoadingComplete() {
        runOnUiThread(() -> {
            loadingScreen.setVisibility(View.GONE);
            uiOverlay.setVisibility(View.VISIBLE);
        });
    }

    // --- SurfaceView Overrides ---

    @Override
    public void onSurfaceCreated(GL10 gl, EGLConfig config) {
        nativeOnSurfaceCreated();
    }

    @Override
    public void onSurfaceChanged(GL10 gl, int width, int height) {
        nativeOnSurfaceChanged(width, height);
    }

    @Override
    public void onDrawFrame(GL10 gl) {
        nativeStep();
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        if (event.getAction() == MotionEvent.ACTION_DOWN) {
            nativeOnTouch(event.getX(), event.getY());
        }
        return true;
    }

    // --- Native Method Declarations ---
    public native void nativeInitAssetManager(Object assetManager);
    public native void nativeOnSurfaceCreated();
    public native void nativeOnSurfaceChanged(int width, int height);
    public native void nativeStep();
    public native void nativeOnTouch(float x, float y);
    public native void nativeOnPlayClicked();
    public native void nativeGrantLives();
}
