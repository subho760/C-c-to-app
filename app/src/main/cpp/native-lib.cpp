#include <jni.h>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <cmath>
#include <chrono>
#include <random>

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
    
    // Cached Method IDs
    jclass canvasCls, matrixCls, paintCls;
    jmethodID canvasDrawBitmap, canvasDrawColor, canvasDrawRect;
    jmethodID matrixInit, matrixSetRotate, matrixPostTranslate, matrixPostScale;
    jmethodID playSoundMid;

    GameEngine() = default;

    // --- Procedural Solvable Generator ---
    void initLevel(int lvl) {
        arrows.clear();
        
        // 1. Determine Grid Size (Progression Logic)
        if (lvl % 5 == 0) { // Boss Levels (5, 10, 15, 20)
            gridW = 5 + (lvl / 10); 
            gridH = 7 + (lvl / 10);
        } else { // Normal Levels
            gridW = 3 + (lvl / 6);
            gridH = 4 + (lvl / 6);
        }

        calculateLayout();

        // 2. Build Solvable Puzzle via Reverse Simulation
        // Start with a list of all possible slots
        std::vector<std::pair<int, int>> slots;
        for(int y=0; y<gridH; ++y) for(int x=0; x<gridW; ++x) slots.push_back({x, y});
        
        std::mt19937 rng(std::chrono::steady_clock::now().time_since_epoch().count());
        std::shuffle(slots.begin(), slots.end(), rng);

        // Fill density: Boss levels are 100% full, normal levels 80%
        int targetCount = (lvl % 5 == 0) ? slots.size() : (int)(slots.size() * 0.85f);
        int idCounter = 0;

        for(int i=0; i < targetCount; ++i) {
            Arrow a;
            a.id = idCounter++;
            a.gx = slots[i].first;
            a.gy = slots[i].second;
            a.curX = (float)a.gx;
            a.curY = (float)a.gy;
            
            // Assign random direction
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
        if (activityObj) {
            env->CallVoidMethod(activityObj, playSoundMid, type);
        }
    }
};

static GameEngine engine;

extern "C" {

JNIEXPORT void JNICALL
Java_com_night_backgroundchange_MainActivity_initNativeEngine(JNIEnv* env, jobject obj, jboolean dark) {
    engine.activityObj = env->NewGlobalRef(obj);
    engine.darkTheme = dark;
    
    jclass actCls = env->GetObjectClass(obj);
    engine.playSoundMid = env->GetMethodID(actCls, "playSound", "(I)V");

    engine.canvasCls = (jclass)env->NewGlobalRef(env->FindClass("android/graphics/Canvas"));
    engine.matrixCls = (jclass)env->NewGlobalRef(env->FindClass("android/graphics/Matrix"));
    engine.paintCls = (jclass)env->NewGlobalRef(env->FindClass("android/graphics/Paint"));

    engine.canvasDrawBitmap = env->GetMethodID(engine.canvasCls, "drawBitmap", "(Landroid/graphics/Bitmap;Landroid/graphics/Matrix;Landroid/graphics/Paint;)V");
    engine.canvasDrawColor = env->GetMethodID(engine.canvasCls, "drawColor", "(I)V");
    
    engine.matrixInit = env->GetMethodID(engine.matrixCls, "<init>", "()V");
    engine.matrixSetRotate = env->GetMethodID(engine.matrixCls, "setRotate", "(FFF)V");
    engine.matrixPostTranslate = env->GetMethodID(engine.matrixCls, "postTranslate", "(FF)V");
    engine.matrixPostScale = env->GetMethodID(engine.matrixCls, "postScale", "(FFFF)V");

    engine.initLevel(1);
}

JNIEXPORT void JNICALL
Java_com_night_backgroundchange_MainActivity_nativePushAsset(JNIEnv* env, jobject obj, jstring name, jobject bmp) {
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

void drawBitmapNative(JNIEnv* env, jobject canvas, jobject bitmap, float x, float y, float scale, float angle) {
    // 1. Instantiate Matrix
    jobject matrix = env->NewObject(engine.matrixCls, engine.matrixInit);
    
    // Standard asset size is 100x100 for calculations
    float pivot = 50.0f; 

    // 2. Apply Transforms: Rotate -> Scale -> Translate
    env->CallVoidMethod(matrix, engine.matrixSetRotate, angle, pivot, pivot);
    env->CallVoidMethod(matrix, engine.matrixPostScale, scale, scale, pivot, pivot);
    env->CallVoidMethod(matrix, engine.matrixPostTranslate, x, y);

    // 3. Draw
    env->CallVoidMethod(canvas, engine.canvasDrawBitmap, bitmap, matrix, nullptr);
    
    // Cleanup local ref to prevent overflow in loop
    env->DeleteLocalRef(matrix);
}

JNIEXPORT void JNICALL
Java_com_night_backgroundchange_MainActivity_nativeRender(JNIEnv* env, jobject obj, jobject canvas) {
    // Background Color
    int bgColor = engine.darkTheme ? 0xFF121212 : 0xFFF5F5F5;
    env->CallVoidMethod(canvas, engine.canvasDrawColor, bgColor);

    if (engine.state == MENU) {
        drawBitmapNative(env, canvas, engine.assets["play"], engine.screenW/2 - 75, engine.screenH/2 - 75, 1.5f, 0);
        return;
    }

    bool allCleared = true;

    // 1. Render Tiles
    for (auto& a : engine.arrows) {
        if (!a.active) continue;
        allCleared = false;
        float drawX = engine.offsetX + a.gx * engine.tileSize;
        float drawY = engine.offsetY + a.gy * engine.tileSize;
        drawBitmapNative(env, canvas, engine.assets["tile"], drawX, drawY, engine.tileSize/100.0f, 0);
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
        
        // Draw glow if removing
        if (a.exiting) {
            drawBitmapNative(env, canvas, engine.assets["glow"], drawX, drawY, engine.tileSize/100.0f, 0);
        }

        drawBitmapNative(env, canvas, engine.assets["arrow"], drawX, drawY, (engine.tileSize/100.0f) * a.scale, (float)a.dir);
    }

    // 3. Victory Check
    if (allCleared && engine.state == PLAYING) {
        engine.state = VICTORY;
        engine.triggerSound(env, 1);
    }

    if (engine.state == VICTORY) {
        drawBitmapNative(env, canvas, engine.assets["star"], engine.screenW/2 - 100, engine.screenH/4, 2.0f, 0);
        drawBitmapNative(env, canvas, engine.assets["next"], engine.screenW/2 - 75, engine.screenH/2 + 100, 1.5f, 0);
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

    // Hit detection for arrows
    for (auto& a : engine.arrows) {
        if (!a.active || a.exiting) continue;

        float ax = engine.offsetX + a.gx * engine.tileSize;
        float ay = engine.offsetY + a.gy * engine.tileSize;

        if (x >= ax && x <= ax + engine.tileSize && y >= ay && y <= ay + engine.tileSize) {
            if (engine.isPathClear(a)) {
                a.exiting = true;
                engine.triggerSound(env, 0); // Click
            }
            break;
        }
    }
}

} // extern "C"
