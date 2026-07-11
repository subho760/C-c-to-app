#include <jni.h>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <algorithm>
#include <cmath>
#include <chrono>
#include <random>

enum Direction { UP = 0, RIGHT = 90, DOWN = 180, LEFT = 270 };
enum GameState { MENU, PLAYING, SETTINGS, VICTORY };

struct Vec2 {
    float x, y;
};

struct Arrow {
    int id;
    int gridX, gridY;
    Direction dir;
    float currentX, currentY;
    float scale = 1.0f;
    float alpha = 1.0f;
    bool isRemoving = false;
    bool isRemoved = false;
};

struct Particle {
    float x, y, vx, vy, life;
};

class GameEngine {
public:
    GameState state = MENU;
    int screenW = 0, screenH = 0;
    int level = 1;
    bool darkTheme = true;
    bool soundOn = true;

    std::vector<Arrow> arrows;
    std::vector<Particle> particles;
    std::map<std::string, jobject> assets;
    
    float tileSize;
    float offsetX, offsetY;
    int gridW = 4, gridH = 5;

    JNIEnv* lastEnv;
    jobject mainActivityObj;
    jclass canvasClass;
    jclass matrixClass;
    jmethodID drawBitmapMid, drawColorMid, drawRectMid;
    jmethodID setRotateMid, postScaleMid, postTranslateMid, matrixInitMid;

    GameEngine() {}

    void initLevel() {
        arrows.clear();
        particles.clear();
        gridW = 3 + (level / 5);
        gridH = 4 + (level / 5);
        if (gridW > 7) gridW = 7;
        if (gridH > 9) gridH = 9;
        
        std::vector<std::pair<int, int>> slots;
        for(int y = 0; y < gridH; y++) {
            for(int x = 0; x < gridW; x++) {
                slots.push_back({x, y});
            }
        }
        
        std::default_random_engine gen(std::chrono::system_clock::now().time_since_epoch().count());
        std::shuffle(slots.begin(), slots.end(), gen);

        int idCounter = 0;
        for(auto& slot : slots) {
            Arrow a;
            a.id = idCounter++;
            a.gridX = slot.first;
            a.gridY = slot.second;
            int d = (std::rand() % 4) * 90;
            a.dir = (Direction)d;
            a.currentX = (float)a.gridX;
            a.currentY = (float)a.gridY;
            arrows.push_back(a);
        }
        recalculateLayout();
    }

    void recalculateLayout() {
        if (screenW <= 0 || screenH <= 0) return;
        tileSize = std::min(screenW / (gridW + 2.0f), screenH / (gridH + 4.0f));
        offsetX = (screenW - (gridW * tileSize)) / 2.0f;
        offsetY = (screenH - (gridH * tileSize)) / 2.0f;
    }

    bool isPathClear(const Arrow& a) {
        for(const auto& other : arrows) {
            if (other.isRemoved || other.isRemoving || other.id == a.id) continue;
            
            if (a.dir == UP && other.gridX == a.gridX && other.gridY < a.gridY) return false;
            if (a.dir == DOWN && other.gridX == a.gridX && other.gridY > a.gridY) return false;
            if (a.dir == LEFT && other.gridY == a.gridY && other.gridX < a.gridX) return false;
            if (a.dir == RIGHT && other.gridY == a.gridY && other.gridX > a.gridX) return false;
        }
        return true;
    }

    void triggerSound(int type) {
        if (!soundOn) return;
        jclass clazz = lastEnv->GetObjectClass(mainActivityObj);
        jmethodID mid = lastEnv->GetMethodID(clazz, "playSound", "(I)V");
        lastEnv->CallVoidMethod(mainActivityObj, mid, type);
    }
};

static GameEngine engine;

void drawBitmap(JNIEnv* env, jobject canvas, jobject bitmap, float x, float y, float scale, float rotation) {
    if (!bitmap) return;
    jobject matrix = env->NewObject(engine.matrixClass, engine.matrixInitMid);
    
    float centerX = 50.0f;
    float centerY = 50.0f;
    
    env->CallVoidMethod(matrix, engine.setRotateMid, rotation, centerX, centerY);
    env->CallVoidMethod(matrix, engine.postScaleMid, scale, scale, centerX, centerY);
    env->CallVoidMethod(matrix, engine.postTranslateMid, x, y);
    
    env->CallVoidMethod(canvas, engine.drawBitmapMid, bitmap, matrix, nullptr);
    env->DeleteLocalRef(matrix);
}

extern "C" {

JNIEXPORT void JNICALL
Java_com_night_backgroundchange_MainActivity_initNative(JNIEnv* env, jobject obj, jboolean dark) {
    engine.mainActivityObj = env->NewGlobalRef(obj);
    engine.darkTheme = dark;
    
    jclass canvasCls = env->FindClass("android/graphics/Canvas");
    engine.canvasClass = (jclass)env->NewGlobalRef(canvasCls);
    
    jclass matrixCls = env->FindClass("android/graphics/Matrix");
    engine.matrixClass = (jclass)env->NewGlobalRef(matrixCls);
    
    engine.matrixInitMid = env->GetMethodID(engine.matrixClass, "<init>", "()V");
    engine.drawBitmapMid = env->GetMethodID(engine.canvasClass, "drawBitmap", "(Landroid/graphics/Bitmap;Landroid/graphics/Matrix;Landroid/graphics/Paint;)V");
    engine.drawColorMid = env->GetMethodID(engine.canvasClass, "drawColor", "(I)V");
    
    engine.setRotateMid = env->GetMethodID(engine.matrixClass, "setRotate", "(FFF)V");
    engine.postScaleMid = env->GetMethodID(engine.matrixClass, "postScale", "(FFFF)V");
    engine.postTranslateMid = env->GetMethodID(engine.matrixClass, "postTranslate", "(FF)V");

    engine.initLevel();
}

JNIEXPORT void JNICALL
Java_com_night_backgroundchange_MainActivity_loadNativeAsset(JNIEnv* env, jobject obj, jstring name, jobject bitmap) {
    const char* nativeName = env->GetStringUTFChars(name, 0);
    engine.assets[std::string(nativeName)] = env->NewGlobalRef(bitmap);
    env->ReleaseStringUTFChars(name, nativeName);
}

JNIEXPORT void JNICALL
Java_com_night_backgroundchange_MainActivity_setNativeSize(JNIEnv* env, jobject obj, jint w, jint h) {
    engine.screenW = w;
    engine.screenH = h;
    engine.recalculateLayout();
}

JNIEXPORT void JNICALL
Java_com_night_backgroundchange_MainActivity_updateAndRenderNative(JNIEnv* env, jobject obj, jobject canvas) {
    engine.lastEnv = env;
    
    int bgColor = engine.darkTheme ? 0xFF121212 : 0xFFF0F0F0;
    env->CallVoidMethod(canvas, engine.drawColorMid, bgColor);

    if (engine.state == PLAYING) {
        bool allCleared = true;
        for (auto& a : engine.arrows) {
            if (a.isRemoved) continue;
            allCleared = false;
            
            if (a.isRemoving) {
                float speed = engine.screenW * 0.04f;
                if (a.dir == UP) a.currentY -= speed / engine.tileSize;
                if (a.dir == DOWN) a.currentY += speed / engine.tileSize;
                if (a.dir == LEFT) a.currentX -= speed / engine.tileSize;
                if (a.dir == RIGHT) a.currentX += speed / engine.tileSize;
                
                a.alpha -= 0.08f;
                a.scale -= 0.04f;
                
                if (a.alpha <= 0.0f || a.scale <= 0.0f) a.isRemoved = true;
            }

            if (!a.isRemoving) {
                drawBitmap(env, canvas, engine.assets["tile"], 
                           engine.offsetX + a.gridX * engine.tileSize, 
                           engine.offsetY + a.gridY * engine.tileSize, 
                           engine.tileSize / 100.0f, 0.0f);
            }

            drawBitmap(env, canvas, engine.assets["arrow"], 
                       engine.offsetX + a.currentX * engine.tileSize, 
                       engine.offsetY + a.currentY * engine.tileSize, 
                       (engine.tileSize / 100.0f) * a.scale, (float)a.dir);
        }
        
        if (allCleared) {
            engine.state = VICTORY;
            engine.triggerSound(1);
        }
    } else if (engine.state == MENU) {
        drawBitmap(env, canvas, engine.assets["play"], engine.screenW / 2.0f - 50.0f, engine.screenH / 2.0f - 50.0f, 1.0f, 0.0f);
    } else if (engine.state == VICTORY) {
        drawBitmap(env, canvas, engine.assets["star"], engine.screenW / 2.0f - 50.0f, engine.screenH / 3.0f, 1.0f, 0.0f);
        drawBitmap(env, canvas, engine.assets["next"], engine.screenW / 2.0f - 50.0f, engine.screenH / 2.0f + 100.0f, 1.0f, 0.0f);
    }
}

JNIEXPORT void JNICALL
Java_com_night_backgroundchange_MainActivity_handleTouchNative(JNIEnv* env, jobject obj, jfloat x, jfloat y) {
    if (engine.state == MENU) {
        engine.state = PLAYING;
        return;
    }
    
    if (engine.state == VICTORY) {
        engine.level++;
        engine.initLevel();
        engine.state = PLAYING;
        return;
    }

    for (auto& a : engine.arrows) {
        if (a.isRemoved || a.isRemoving) continue;
        
        float ax = engine.offsetX + a.gridX * engine.tileSize;
        float ay = engine.offsetY + a.gridY * engine.tileSize;
        
        if (x >= ax && x <= ax + engine.tileSize && y >= ay && y <= ay + engine.tileSize) {
            if (engine.isPathClear(a)) {
                a.isRemoving = true;
                engine.triggerSound(0);
            }
            break;
        }
    }
}

}
