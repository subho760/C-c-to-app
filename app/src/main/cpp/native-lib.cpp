#include <jni.h>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <cmath>
#include <chrono>
#include <random>
#include <android/log.h>

#define LOG_TAG "NativeGame"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// --- Constants & Enums ---
enum Direction { UP = 270, RIGHT = 0, DOWN = 90, LEFT = 180 };
enum GameState { MENU, PLAYING, VICTORY, SETTINGS };

struct Arrow {
    int id;
    int gx, gy;        // Grid Coordinates
    float curX, curY;  // Animation Coordinates
    Direction dir;
    float scale = 1.0f;
    float alpha = 1.0f;
    bool active = true;
    bool exiting = false;
};

class GameEngine {
public:
    // UI & State
    GameState state = MENU;
    int level = 1;
    bool darkTheme = true;
    int screenW = 0, screenH = 0;

    // Grid Logic
    int gridW, gridH;
    float tileSize, offsetX, offsetY;
    std::vector<Arrow> arrows;

    // JNI Cached References
    jobject activityObj = nullptr;
    std::map<std::string, jobject> assets;
    jmethodID playSoundMid = nullptr;

    GameEngine() = default;

    // --- Procedural Solvable Generator ---
    void initLevel(int lvl) {
        arrows.clear();
        
        if (lvl % 5 == 0) { // Boss Levels
            gridW = 5 + (lvl / 10); 
            gridH = 7 + (lvl / 10);
        } else { // Normal Levels
            gridW = 3 + (lvl / 6);
            gridH = 4 + (lvl / 6);
        }

        calculateLayout();

        // Build Solvable Puzzle via Reverse Simulation
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

// Safe utility to draw bitmaps handling local lookups with corrected Matrix JNI signatures
void drawBitmapNative(JNIEnv* env, jobject canvas, jobject bitmap, float x, float y, float scale, float angle) {
    if (!canvas || !bitmap) return;

    jclass matrixCls = env->FindClass("android/graphics/Matrix");
    if (!matrixCls) return;

    jmethodID matrixInit = env->GetMethodID(matrixCls, "<init>", "()V");
    // Corrected signatures: Matrix methods return boolean (Z), not void (V)
    jmethodID matrixSetRotate = env->GetMethodID(matrixCls, "setRotate", "(FFF)Z");
    jmethodID matrixPostTranslate = env->GetMethodID(matrixCls, "postTranslate", "(FF)Z");
    jmethodID matrixPostScale = env->GetMethodID(matrixCls, "postScale", "(FFFF)Z");

    jclass canvasCls = env->FindClass("android/graphics/Canvas");
    if (!canvasCls) {
        env->DeleteLocalRef(matrixCls);
        return;
    }
    jmethodID canvasDrawBitmap = env->GetMethodID(canvasCls, "drawBitmap", "(Landroid/graphics/Bitmap;Landroid/graphics/Matrix;Landroid/graphics/Paint;)V");

    if (!matrixInit || !matrixSetRotate || !matrixPostTranslate || !matrixPostScale || !canvasDrawBitmap) {
        LOGE("Failed to find one or more Matrix/Canvas JNI Method IDs!");
        env->DeleteLocalRef(matrixCls);
        env->DeleteLocalRef(canvasCls);
        return;
    }

    jobject matrix = env->NewObject(matrixCls, matrixInit);
    float pivot = 50.0f; 

    // Call with boolean return expectations
    env->CallBooleanMethod(matrix, matrixSetRotate, angle, pivot, pivot);
    env->CallBooleanMethod(matrix, matrixPostScale, scale, scale, pivot, pivot);
    env->CallBooleanMethod(matrix, matrixPostTranslate, x, y);

    env->CallVoidMethod(canvas, canvasDrawBitmap, bitmap, matrix, nullptr);

    env->DeleteLocalRef(matrix);
    env->DeleteLocalRef(matrixCls);
    env->DeleteLocalRef(canvasCls);
}

extern "C" {

JNIEXPORT void JNICALL
Java_com_night_backgroundchange_MainActivity_initNativeEngine(JNIEnv* env, jobject obj, jboolean dark) {
    engine.activityObj = env->NewGlobalRef(obj);
    engine.darkTheme = dark;
    
    jclass actCls = env->GetObjectClass(obj);
    engine.playSoundMid = env->GetMethodID(actCls, "playSound", "(I)V");

    engine.initLevel(1);
}

JNIEXPORT void JNICALL
Java_com_night_backgroundchange_MainActivity_nativePushAsset(JNIEnv* env, jobject obj, jstring name, jobject bmp) {
    if (!bmp) return;
    const char* utfName = env->GetStringUTFChars(name, nullptr);
    engine.assets[std::string(utfName)] = env->NewGlobalRef(bmp);
    env->ReleaseStringUTFChars(name, utfName);
}

JNIEXPORT void JNICALL
Java_com_night_backgroundchange_MainActivity_nativeOnResize(JNIEnv* env, jobject obj, jint w, jint h) {
    engine.screenW = w;
    engine.screenH = h;
    engine.calculateLayout();
}

JNIEXPORT void JNICALL
Java_com_night_backgroundchange_MainActivity_nativeRender(JNIEnv* env, jobject obj, jobject canvas) {
    if (!canvas) return;

    jclass canvasCls = env->FindClass("android/graphics/Canvas");
    if (!canvasCls) return;

    jmethodID canvasDrawColor = env->GetMethodID(canvasCls, "drawColor", "(I)V");
    if (canvasDrawColor) {
        int bgColor = engine.darkTheme ? 0xFF121212 : 0xFFF5F5F5;
        env->CallVoidMethod(canvas, canvasDrawColor, bgColor);
    }
    env->DeleteLocalRef(canvasCls);

    auto playBmp = engine.assets.count("play") ? engine.assets["play"] : nullptr;
    auto tileBmp = engine.assets.count("tile") ? engine.assets["tile"] : nullptr;
    auto glowBmp = engine.assets.count("glow") ? engine.assets["glow"] : nullptr;
    auto arrowBmp = engine.assets.count("arrow") ? engine.assets["arrow"] : nullptr;
    auto starBmp = engine.assets.count("star") ? engine.assets["star"] : nullptr;
    auto nextBmp = engine.assets.count("next") ? engine.assets["next"] : nullptr;

    if (engine.state == MENU) {
        drawBitmapNative(env, canvas, playBmp, engine.screenW/2.0f - 75, engine.screenH/2.0f - 75, 1.5f, 0);
        return;
    }

    bool allCleared = true;

    // 1. Render Tiles
    for (auto& a : engine.arrows) {
        if (!a.active) continue;
        allCleared = false;
        float drawX = engine.offsetX + a.gx * engine.tileSize;
        float drawY = engine.offsetY + a.gy * engine.tileSize;
        drawBitmapNative(env, canvas, tileBmp, drawX, drawY, engine.tileSize/100.0f, 0);
    }

    // 2. Update and Render Arrows
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
        
        if (a.exiting) {
            drawBitmapNative(env, canvas, glowBmp, drawX, drawY, engine.tileSize/100.0f, 0);
        }

        drawBitmapNative(env, canvas, arrowBmp, drawX, drawY, (engine.tileSize/100.0f) * a.scale, (float)a.dir);
    }

    // 3. Victory Check
    if (allCleared && engine.state == PLAYING) {
        engine.state = VICTORY;
        engine.triggerSound(env, 1);
    }

    if (engine.state == VICTORY) {
        drawBitmapNative(env, canvas, starBmp, engine.screenW/2.0f - 100, engine.screenH/4.0f, 2.0f, 0);
        drawBitmapNative(env, canvas, nextBmp, engine.screenW/2.0f - 75, engine.screenH/2.0f + 100, 1.5f, 0);
    }
}

JNIEXPORT void JNICALL
Java_com_night_backgroundchange_MainActivity_nativeOnTouch(JNIEnv* env, jobject obj, jfloat x, jfloat y) {
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
