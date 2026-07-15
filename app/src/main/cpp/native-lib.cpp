#include <jni.h>
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>
#include <chrono>
#include <random>
#include <android/log.h>

#define LOG_TAG "NativeGame"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

enum Direction { UP = 270, RIGHT = 0, DOWN = 90, LEFT = 180 };
enum GameState { MENU, PLAYING, VICTORY, SETTINGS };

struct Arrow {
    int id;
    int gx, gy;
    float curX, curY;
    Direction dir;
    float scale = 1.0f;
    float alpha = 1.0f;
    bool active = true;
    bool exiting = false;
};

enum AssetIndex {
    ASSET_ARROW = 0, ASSET_TILE, ASSET_GLOW, ASSET_BACK, ASSET_HOME,
    ASSET_RETRY, ASSET_NEXT, ASSET_PLAY, ASSET_PAUSED, ASSET_SETTINGS,
    ASSET_SOUND_ON, ASSET_SOUND_OFF, ASSET_TICK, ASSET_STAR, ASSET_HINT,
    ASSET_CLOSE, ASSET_LOCK, ASSET_COUNT
};

class GameEngine {
public:
    GameState state = MENU;
    int level = 1;
    bool darkTheme = true;
    int screenW = 0, screenH = 0;
    bool ready = false;

    int gridW = 3, gridH = 4;
    float tileSize = 0, offsetX = 0, offsetY = 0;
    std::vector<Arrow> arrows;

    jobject activityObj = nullptr;
    jobject assets[ASSET_COUNT] = { nullptr };
    
    jclass canvasCls = nullptr;
    
    jmethodID canvasDrawColor = nullptr;
    jmethodID canvasSave = nullptr;
    jmethodID canvasTranslate = nullptr;
    jmethodID canvasRotate = nullptr;
    jmethodID canvasScale = nullptr;
    jmethodID canvasDrawBitmap = nullptr;
    jmethodID canvasRestore = nullptr;
    jmethodID playSoundMid = nullptr;

    GameEngine() = default;

    void initLevel(int lvl) {
        arrows.clear();
        if (lvl % 5 == 0) {
            gridW = 5 + (lvl / 10); 
            gridH = 7 + (lvl / 10);
        } else {
            gridW = 3 + (lvl / 6);
            gridH = 4 + (lvl / 6);
        }
        calculateLayout();

        std::vector<std::pair<int, int>> slots;
        for(int y=0; y<gridH; ++y) for(int x=0; x<gridW; ++x) slots.push_back({x, y});
        
        std::mt19937 rng(std::chrono::steady_clock::now().time_since_epoch().count());
        std::shuffle(slots.begin(), slots.end(), rng);

        int targetCount = (lvl % 5 == 0) ? slots.size() : (int)(slots.size() * 0.85f);
        int idCounter = 0;

        for(int i=0; i < targetCount; ++i) {
            Arrow a;
            a.id = idCounter++;
            a.gx = slots[i].first;
            a.gy = slots[i].second;
            a.curX = (float)a.gx;
            a.curY = (float)a.gy;
            
            int d = std::uniform_int_distribution<int>(0, 3)(rng);
            if(d == 0) a.dir = UP;
            else if(d == 1) a.dir = RIGHT;
            else if(d == 2) a.dir = DOWN;
            else a.dir = LEFT;
            
            arrows.push_back(a);
        }
    }

    void calculateLayout() {
        if (screenW == 0 || screenH == 0) return;
        float margin = screenW * 0.1f;
        tileSize = std::min((screenW - margin) / gridW, (screenH - margin * 4) / gridH);
        offsetX = (screenW - (gridW * tileSize)) / 2.0f;
        offsetY = (screenH - (gridH * tileSize)) / 2.0f;
    }

    bool isPathClear(const Arrow& subject) {
        for(const auto& other : arrows) {
            if (!other.active || other.exiting || other.id == subject.id) continue;
            if (subject.dir == UP && other.gx == subject.gx && other.gy < subject.gy) return false;
            if (subject.dir == DOWN && other.gx == subject.gx && other.gy > subject.gy) return false;
            if (subject.dir == LEFT && other.gy == subject.gy && other.gx < subject.gx) return false;
            if (subject.dir == RIGHT && other.gy == subject.gy && other.gx > subject.gx) return false;
        }
        return true;
    }

    void triggerSound(JNIEnv* env, int type) {
        if (activityObj && playSoundMid) {
            env->CallVoidMethod(activityObj, playSoundMid, type);
        }
    }
};

static GameEngine engine;

void drawBitmapNative(JNIEnv* env, jobject canvas, jobject bitmap, float x, float y, float scale, float angle) {
    if (!canvas || !bitmap || !engine.canvasSave || !engine.canvasTranslate || 
        !engine.canvasRotate || !engine.canvasScale || !engine.canvasDrawBitmap || !engine.canvasRestore) return;

    // Use pure Canvas transformations to prevent reference allocation drops
    env->CallIntMethod(canvas, engine.canvasSave);
    
    env->CallVoidMethod(canvas, engine.canvasTranslate, x, y);
    
    float pivot = 50.0f;
    env->CallVoidMethod(canvas, engine.canvasRotate, angle, pivot, pivot);
    env->CallVoidMethod(canvas, engine.canvasScale, scale, scale, pivot, pivot);
    
    env->CallVoidMethod(canvas, engine.canvasDrawBitmap, bitmap, 0.0f, 0.0f, nullptr);
    
    env->CallVoidMethod(canvas, engine.canvasRestore);
}

extern "C" {

JNIEXPORT void JNICALL
Java_com_night_backgroundchange_MainActivity_initNativeEngine(JNIEnv* env, jobject obj, jboolean dark) {
    engine.ready = false;
    engine.activityObj = env->NewGlobalRef(obj);
    engine.darkTheme = dark;
    
    jclass actCls = env->GetObjectClass(obj);
    if (actCls) {
        engine.playSoundMid = env->GetMethodID(actCls, "playSound", "(I)V");
    }

    jclass localCanvasCls = env->FindClass("android/graphics/Canvas");
    if (localCanvasCls) {
        engine.canvasCls = (jclass)env->NewGlobalRef(localCanvasCls);
        env->DeleteLocalRef(localCanvasCls);
    }

    if (engine.canvasCls) {
        engine.canvasDrawColor = env->GetMethodID(engine.canvasCls, "drawColor", "(I)V");
        engine.canvasSave = env->GetMethodID(engine.canvasCls, "save", "()I");
        engine.canvasTranslate = env->GetMethodID(engine.canvasCls, "translate", "(FF)V");
        engine.canvasRotate = env->GetMethodID(engine.canvasCls, "rotate", "(FFF)V");
        engine.canvasScale = env->GetMethodID(engine.canvasCls, "scale", "(FFFF)V");
        engine.canvasDrawBitmap = env->GetMethodID(engine.canvasCls, "drawBitmap", "(Landroid/graphics/Bitmap;FFLandroid/graphics/Paint;)V");
        engine.canvasRestore = env->GetMethodID(engine.canvasCls, "restore", "()V");
    }

    engine.initLevel(1);
    engine.ready = true;
}

JNIEXPORT void JNICALL
Java_com_night_backgroundchange_MainActivity_nativePushAsset(JNIEnv* env, jobject obj, jint index, jobject bmp) {
    if (index >= 0 && index < ASSET_COUNT && bmp) {
        if (engine.assets[index] != nullptr) {
            env->DeleteGlobalRef(engine.assets[index]);
        }
        engine.assets[index] = env->NewGlobalRef(bmp);
    }
}

JNIEXPORT void JNICALL
Java_com_night_backgroundchange_MainActivity_nativeOnResize(JNIEnv* env, jobject obj, jint w, jint h) {
    engine.screenW = w;
    engine.screenH = h;
    engine.calculateLayout();
}

JNIEXPORT void JNICALL
Java_com_night_backgroundchange_MainActivity_nativeRender(JNIEnv* env, jobject obj, jobject canvas) {
    if (!canvas || !engine.ready || !engine.canvasDrawColor) return;

    int bgColor = engine.darkTheme ? 0xFF121212 : 0xFFF5F5F5;
    env->CallVoidMethod(canvas, engine.canvasDrawColor, bgColor);

    jobject playBmp  = engine.assets[ASSET_PLAY];
    jobject tileBmp  = engine.assets[ASSET_TILE];
    jobject glowBmp  = engine.assets[ASSET_GLOW];
    jobject arrowBmp = engine.assets[ASSET_ARROW];
    jobject starBmp  = engine.assets[ASSET_STAR];
    jobject nextBmp  = engine.assets[ASSET_NEXT];

    if (engine.state == MENU) {
        if (playBmp) drawBitmapNative(env, canvas, playBmp, engine.screenW/2.0f - 75, engine.screenH/2.0f - 75, 1.5f, 0);
        return;
    }

    bool allCleared = true;

    // 1. Render Background Tiles
    for (auto& a : engine.arrows) {
        if (!a.active) continue;
        allCleared = false;
        float drawX = engine.offsetX + a.gx * engine.tileSize;
        float drawY = engine.offsetY + a.gy * engine.tileSize;
        if (tileBmp) drawBitmapNative(env, canvas, tileBmp, drawX, drawY, engine.tileSize/100.0f, 0);
    }

    // 2. Render Interactive Arrows
    for (auto& a : engine.arrows) {
        if (!a.active) continue;

        if (a.exiting) {
            float speed = 0.4f;
            if (a.dir == UP) a.curY -= speed;
            else if (a.dir == DOWN) a.curY += speed;
            else if (a.dir == LEFT) a.curX -= speed;
            else if (a.dir == RIGHT) a.curX += speed;
            
            a.alpha -= 0.05f;
            a.scale -= 0.04f;
            if (a.alpha <= 0) a.active = false;
        }

        float drawX = engine.offsetX + a.curX * engine.tileSize;
        float drawY = engine.offsetY + a.curY * engine.tileSize;
        
        if (a.exiting && glowBmp) {
            drawBitmapNative(env, canvas, glowBmp, drawX, drawY, engine.tileSize/100.0f, 0);
        }

        if (arrowBmp) {
            drawBitmapNative(env, canvas, arrowBmp, drawX, drawY, (engine.tileSize/100.0f) * a.scale, (float)a.dir);
        }
    }

    if (allCleared && engine.state == PLAYING) {
        engine.state = VICTORY;
        engine.triggerSound(env, 1);
    }

    if (engine.state == VICTORY) {
        if (starBmp) drawBitmapNative(env, canvas, starBmp, engine.screenW/2.0f - 100, engine.screenH/4.0f, 2.0f, 0);
        if (nextBmp) drawBitmapNative(env, canvas, nextBmp, engine.screenW/2.0f - 75, engine.screenH/2.0f + 100, 1.5f, 0);
    }
}

JNIEXPORT void JNICALL
Java_com_night_backgroundchange_MainActivity_nativeOnTouch(JNIEnv* env, jobject obj, jfloat x, jfloat y) {
    if (!engine.ready) return;
    
    if (engine.state == MENU) {
        engine.state = PLAYING;
        return;
    }

    if (engine.state == VICTORY) {
        engine.level++;
        engine.initLevel(engine.level);
        engine.state = PLAYING;
        return;
    }

    for (auto& a : engine.arrows) {
        if (!a.active || a.exiting) continue;

        float ax = engine.offsetX + a.gx * engine.tileSize;
        float ay = engine.offsetY + a.gy * engine.tileSize;

        if (x >= ax && x <= ax + engine.tileSize && y >= ay && y <= ay + engine.tileSize) {
            if (engine.isPathClear(a)) {
                a.exiting = true;
                engine.triggerSound(env, 0); 
            }
            break;
        }
    }
}

} // extern "C"
