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
enum GameState { MENU, LEVEL_SELECT, PLAYING, VICTORY, SETTINGS };

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

struct UIButton {
    float x, y, w, h;
    int actionId; 
};

class GameEngine {
public:
    GameState state = MENU;
    int level = 1;
    int maxUnlockedLevel = 1;
    bool soundEnabled = true;
    bool darkTheme = true;
    int screenW = 0, screenH = 0;
    bool ready = false;

    int gridW = 3, gridH = 4;
    float tileSize = 0, offsetX = 0, offsetY = 0;
    std::vector<Arrow> arrows;
    std::vector<UIButton> activeButtons;

    jobject activityObj = nullptr;
    jobject assets[ASSET_COUNT] = { nullptr };
    
    jclass canvasCls = nullptr;
    jclass bitmapCls = nullptr;
    jclass paintCls = nullptr;
    
    jmethodID canvasDrawColor = nullptr;
    jmethodID canvasSave = nullptr;
    jmethodID canvasTranslate = nullptr;
    jmethodID canvasRotate = nullptr;
    jmethodID canvasScale = nullptr;
    jmethodID canvasDrawBitmap = nullptr;
    jmethodID canvasRestore = nullptr;
    jmethodID canvasDrawText = nullptr;
    jmethodID bitmapGetWidth = nullptr;
    jmethodID bitmapGetHeight = nullptr;
    jmethodID playSoundMid = nullptr;
    jobject textPaint = nullptr;

    GameEngine() = default;

    void initLevel(int lvl) {
        level = lvl;
        arrows.clear();
        
        // Dynamic grid generation mimicking the progression in the reference video
        if (lvl <= 3) { gridW = 3; gridH = 3; }
        else if (lvl <= 8) { gridW = 3; gridH = 4; }
        else if (lvl <= 15) { gridW = 4; gridH = 5; }
        else { gridW = 5; gridH = 6; }
        
        calculateLayout();

        std::vector<std::pair<int, int>> slots;
        for(int y=0; y<gridH; ++y) {
            for(int x=0; x<gridW; ++x) {
                // Leave procedural empty spaces inside layout borders to make advanced puzzle patterns
                if ((x == 0 && y == 0) || (x == gridW-1 && y == gridH-1)) continue;
                slots.push_back({x, y});
            }
        }
        
        std::mt19937 rng(std::chrono::steady_clock::now().time_since_epoch().count());
        std::shuffle(slots.begin(), slots.end(), rng);

        int targetCount = std::min((int)slots.size(), 4 + (lvl * 2));
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
        float margin = screenW * 0.12f;
        tileSize = std::min((screenW - margin) / gridW, (screenH - margin * 5) / gridH);
        offsetX = (screenW - (gridW * tileSize)) / 2.0f;
        offsetY = (screenH - (gridH * tileSize)) / 2.2f; // Push layout slightly up to clear bottom menu bounds
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
        if (activityObj && playSoundMid && soundEnabled) {
            env->CallVoidMethod(activityObj, playSoundMid, type);
        }
    }
};

static GameEngine engine;

void drawBitmapNative(JNIEnv* env, jobject canvas, jobject bitmap, float x, float y, float targetWidth, float angle) {
    if (!canvas || !bitmap || !engine.canvasSave || !engine.canvasTranslate || 
        !engine.canvasRotate || !engine.canvasScale || !engine.canvasDrawBitmap || !engine.canvasRestore) return;

    jint bmpW = 100, bmpH = 100;
    if (engine.bitmapGetWidth) bmpW = env->CallIntMethod(bitmap, engine.bitmapGetWidth);
    if (engine.bitmapGetHeight) bmpH = env->CallIntMethod(bitmap, engine.bitmapGetHeight);

    float scale = targetWidth / (float)bmpW;
    jfloat pivotX = (jfloat)bmpW / 2.0f;
    jfloat pivotY = (jfloat)bmpH / 2.0f;

    env->CallIntMethod(canvas, engine.canvasSave);
    env->CallVoidMethod(canvas, engine.canvasTranslate, (jfloat)x, (jfloat)y);
    env->CallVoidMethod(canvas, engine.canvasRotate, (jfloat)angle, pivotX, pivotY);
    env->CallVoidMethod(canvas, engine.canvasScale, (jfloat)scale, (jfloat)scale, pivotX, pivotY);
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
    if (actCls) engine.playSoundMid = env->GetMethodID(actCls, "playSound", "(I)V");

    jclass localCanvasCls = env->FindClass("android/graphics/Canvas");
    if (localCanvasCls) {
        engine.canvasCls = (jclass)env->NewGlobalRef(localCanvasCls);
        env->DeleteLocalRef(localCanvasCls);
    }
    jclass localBitmapCls = env->FindClass("android/graphics/Bitmap");
    if (localBitmapCls) {
        engine.bitmapCls = (jclass)env->NewGlobalRef(localBitmapCls);
        env->DeleteLocalRef(localBitmapCls);
    }
    jclass localPaintCls = env->FindClass("android/graphics/Paint");
    if (localPaintCls) {
        engine.paintCls = (jclass)env->NewGlobalRef(localPaintCls);
        env->DeleteLocalRef(localPaintCls);
    }

    if (engine.canvasCls) {
        engine.canvasDrawColor = env->GetMethodID(engine.canvasCls, "drawColor", "(I)V");
        engine.canvasSave = env->GetMethodID(engine.canvasCls, "save", "()I");
        engine.canvasTranslate = env->GetMethodID(engine.canvasCls, "translate", "(FF)V");
        engine.canvasRotate = env->GetMethodID(engine.canvasCls, "rotate", "(FFF)V");
        engine.canvasScale = env->GetMethodID(engine.canvasCls, "scale", "(FFFF)V");
        engine.canvasDrawBitmap = env->GetMethodID(engine.canvasCls, "drawBitmap", "(Landroid/graphics/Bitmap;FFLandroid/graphics/Paint;)V");
        engine.canvasRestore = env->GetMethodID(engine.canvasCls, "restore", "()V");
        engine.canvasDrawText = env->GetMethodID(engine.canvasCls, "drawText", "(Ljava/lang/String;FFLandroid/graphics/Paint;)V");
    }
    if (engine.bitmapCls) {
        engine.bitmapGetWidth = env->GetMethodID(engine.bitmapCls, "getWidth", "()I");
        engine.bitmapGetHeight = env->GetMethodID(engine.bitmapCls, "getHeight", "()I");
    }

    // Allocate textual painting properties native instance
    if (engine.paintCls) {
        jmethodID paintInit = env->GetMethodID(engine.paintCls, "<init>", "()V");
        jobject localPaint = env->NewObject(engine.paintCls, paintInit);
        jmethodID setAntiAlias = env->GetMethodID(engine.paintCls, "setAntiAlias", "(Z)V");
        jmethodID setColor = env->GetMethodID(engine.paintCls, "setColor", "(I)V");
        jmethodID setTextSize = env->GetMethodID(engine.paintCls, "setTextSize", "(F)V");
        
        env->CallVoidMethod(localPaint, setAntiAlias, JNI_TRUE);
        env->CallVoidMethod(localPaint, setColor, 0xFFFFFFFF);
        env->CallVoidMethod(localPaint, setTextSize, 48.0f);
        engine.textPaint = env->NewGlobalRef(localPaint);
        env->DeleteLocalRef(localPaint);
    }

    engine.initLevel(1);
    engine.ready = true;
}

JNIEXPORT void JNICALL
Java_com_night_backgroundchange_MainActivity_nativePushAsset(JNIEnv* env, jobject obj, jint index, jobject bmp) {
    if (index >= 0 && index < ASSET_COUNT && bmp) {
        if (engine.assets[index] != nullptr) env->DeleteGlobalRef(engine.assets[index]);
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

    int bgColor = engine.darkTheme ? 0xFF0F172A : 0xFFF8FAFC; // Dark Slate Theme from Reference Video
    env->CallVoidMethod(canvas, engine.canvasDrawColor, bgColor);
    engine.activeButtons.clear();

    // --- STATE MACHINE 1: MAIN HOME MENU ---
    if (engine.state == MENU) {
        float logoSize = engine.screenW * 0.65f;
        if (engine.assets[ASSET_ARROW]) {
            drawBitmapNative(env, canvas, engine.assets[ASSET_ARROW], engine.screenW/2.0f - logoSize/2.0f, engine.screenH * 0.2f, logoSize, 0);
        }

        float btnSize = engine.screenW * 0.35f;
        float btnX = engine.screenW/2.0f - btnSize/2.0f;
        float btnY = engine.screenH * 0.55f;
        if (engine.assets[ASSET_PLAY]) {
            drawBitmapNative(env, canvas, engine.assets[ASSET_PLAY], btnX, btnY, btnSize, 0);
            engine.activeButtons.push_back({btnX, btnY, btnSize, btnSize, 101}); // Action 101: Go to level selection
        }

        float setSize = engine.screenW * 0.15f;
        float setX = engine.screenW/2.0f - setSize/2.0f;
        float setY = engine.screenH * 0.8f;
        if (engine.assets[ASSET_SETTINGS]) {
            drawBitmapNative(env, canvas, engine.assets[ASSET_SETTINGS], setX, setY, setSize, 0);
            engine.activeButtons.push_back({setX, setY, setSize, setSize, 102}); // Action 102: Settings
        }
        return;
    }

    // --- STATE MACHINE 2: LEVEL SELECTION PAGE ---
    if (engine.state == LEVEL_SELECT) {
        float closeSize = engine.screenW * 0.12f;
        if (engine.assets[ASSET_CLOSE]) {
            drawBitmapNative(env, canvas, engine.assets[ASSET_CLOSE], 40, 60, closeSize, 0);
            engine.activeButtons.push_back({40, 60, closeSize, closeSize, 201}); // Action 201: Back to main menu
        }

        // Draw level selection boxes grid layout
        int cols = 4;
        float boxSize = engine.screenW * 0.16f;
        float spacing = engine.screenW * 0.05f;
        float startGridX = (engine.screenW - (cols * boxSize + (cols-1) * spacing)) / 2.0f;
        float startGridY = engine.screenH * 0.22f;

        for (int i = 0; i < 20; i++) {
            int r = i / cols;
            int c = i % cols;
            float bx = startGridX + c * (boxSize + spacing);
            float by = startGridY + r * (boxSize + spacing);
            int currentBoxLvl = i + 1;

            if (currentBoxLvl <= engine.maxUnlockedLevel) {
                if (engine.assets[ASSET_TILE]) drawBitmapNative(env, canvas, engine.assets[ASSET_TILE], bx, by, boxSize, 0);
                engine.activeButtons.push_back({bx, by, boxSize, boxSize, currentBoxLvl}); // Action 1-20: Start Level
            } else {
                if (engine.assets[ASSET_LOCK]) drawBitmapNative(env, canvas, engine.assets[ASSET_LOCK], bx, by, boxSize, 0);
            }
        }
        return;
    }

    // --- STATE MACHINE 3: SETTINGS VIEW ---
    if (engine.state == SETTINGS) {
        float closeSize = engine.screenW * 0.12f;
        if (engine.assets[ASSET_CLOSE]) {
            drawBitmapNative(env, canvas, engine.assets[ASSET_CLOSE], engine.screenW - closeSize - 40, 60, closeSize, 0);
            engine.activeButtons.push_back({engine.screenW - closeSize - 40, 60, closeSize, closeSize, 201});
        }

        float soundBtnSize = engine.screenW * 0.25f;
        float soundX = engine.screenW/2.0f - soundBtnSize/2.0f;
        float soundY = engine.screenH * 0.4f;
        jobject soundAsset = engine.soundEnabled ? engine.assets[ASSET_SOUND_ON] : engine.assets[ASSET_SOUND_OFF];
        if (soundAsset) {
            drawBitmapNative(env, canvas, soundAsset, soundX, soundY, soundBtnSize, 0);
            engine.activeButtons.push_back({soundX, soundY, soundBtnSize, soundBtnSize, 301}); // Action 301: Toggle Audio
        }
        return;
    }

    // --- STATE MACHINE 4: ACTIVE PUZZLE GAMEPLAY ---
    // Top HUD Bar Rendering
    float hudSize = engine.screenW * 0.10f;
    if (engine.assets[ASSET_HOME]) {
        drawBitmapNative(env, canvas, engine.assets[ASSET_HOME], 40, 50, hudSize, 0);
        engine.activeButtons.push_back({40, 50, hudSize, hudSize, 202}); // Action 202: Level Selector
    }
    if (engine.assets[ASSET_RETRY]) {
        drawBitmapNative(env, canvas, engine.assets[ASSET_RETRY], engine.screenW - hudSize - 40, 50, hudSize, 0);
        engine.activeButtons.push_back({engine.screenW - hudSize - 40, 50, hudSize, hudSize, 203}); // Action 203: Refresh level
    }

    bool allCleared = true;

    // Render underlying puzzle tiles grid
    for (auto& a : engine.arrows) {
        if (!a.active) continue;
        allCleared = false;
        float drawX = engine.offsetX + a.gx * engine.tileSize;
        float drawY = engine.offsetY + a.gy * engine.tileSize;
        if (engine.assets[ASSET_TILE]) drawBitmapNative(env, canvas, engine.assets[ASSET_TILE], drawX, drawY, engine.tileSize, 0);
    }

    // Render puzzle arrows with movement paths
    for (auto& a : engine.arrows) {
        if (!a.active) continue;

        if (a.exiting) {
            float speed = 0.35f;
            if (a.dir == UP) a.curY -= speed;
            else if (a.dir == DOWN) a.curY += speed;
            else if (a.dir == LEFT) a.curX -= speed;
            else if (a.dir == RIGHT) a.curX += speed;
            
            a.alpha -= 0.06f;
            a.scale -= 0.05f;
            if (a.alpha <= 0) a.active = false;
        }

        float drawX = engine.offsetX + a.curX * engine.tileSize;
        float drawY = engine.offsetY + a.curY * engine.tileSize;
        
        if (a.exiting && engine.assets[ASSET_GLOW]) {
            drawBitmapNative(env, canvas, engine.assets[ASSET_GLOW], drawX, drawY, engine.tileSize, 0);
        }
        if (engine.assets[ASSET_ARROW]) {
            drawBitmapNative(env, canvas, engine.assets[ASSET_ARROW], drawX, drawY, engine.tileSize * a.scale, (float)a.dir);
        }
    }

    if (allCleared && engine.state == PLAYING) {
        engine.state = VICTORY;
        if (engine.level == engine.maxUnlockedLevel && engine.maxUnlockedLevel < 20) {
            engine.maxUnlockedLevel++;
        }
        engine.triggerSound(env, 1);
    }

    // --- STATE MACHINE 5: LEVEL VICTORY OVERLAY ---
    if (engine.state == VICTORY) {
        float starSize = engine.screenW * 0.45f;
        float nextSize = engine.screenW * 0.28f;
        float starX = engine.screenW / 2.0f - starSize / 2.0f;
        float starY = engine.screenH / 4.0f;
        float nextX = engine.screenW / 2.0f - nextSize / 2.0f;
        float nextY = engine.screenH / 2.0f + 120;

        if (engine.assets[ASSET_STAR]) drawBitmapNative(env, canvas, engine.assets[ASSET_STAR], starX, starY, starSize, 0);
        if (engine.assets[ASSET_NEXT]) {
            drawBitmapNative(env, canvas, engine.assets[ASSET_NEXT], nextX, nextY, nextSize, 0);
            engine.activeButtons.push_back({nextX, nextY, nextSize, nextSize, 401}); // Action 401: Proceed to next level
        }
    }
}

JNIEXPORT void JNICALL
Java_com_night_backgroundchange_MainActivity_nativeOnTouch(JNIEnv* env, jobject obj, jfloat x, jfloat y) {
    if (!engine.ready) return;
    
    // Check custom bound UI buttons intercept actions first
    for (const auto& btn : engine.activeButtons) {
        if (x >= btn.x && x <= btn.x + btn.w && y >= btn.y && y <= btn.y + btn.h) {
            engine.triggerSound(env, 0);
            
            if (btn.actionId == 101) { engine.state = LEVEL_SELECT; }
            else if (btn.actionId == 102) { engine.state = SETTINGS; }
            else if (btn.actionId == 201) { engine.state = MENU; }
            else if (btn.actionId == 202) { engine.state = LEVEL_SELECT; }
            else if (btn.actionId == 203) { engine.initLevel(engine.level); engine.state = PLAYING; }
            else if (btn.actionId == 301) { engine.soundEnabled = !engine.soundEnabled; }
            else if (btn.actionId == 401) { engine.level++; engine.initLevel(engine.level); engine.state = PLAYING; }
            else if (btn.actionId >= 1 && btn.actionId <= 20) {
                engine.initLevel(btn.actionId);
                engine.state = PLAYING;
            }
            return;
        }
    }

    // Standard arrow touch collision box maps
    if (engine.state == PLAYING) {
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
}

} // extern "C"
