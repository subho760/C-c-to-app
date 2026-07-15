#include <jni.h>
#include <string>
#include <vector>
#include <android/log.h>

#define LOG_TAG "GameUI"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// Clean state structure definitions
enum GameState { STATE_LOADING, STATE_HOME, STATE_SETTINGS, STATE_GAMEPLAY };

enum AssetIndex {
    ASSET_ARROW = 0, ASSET_TILE, ASSET_GLOW, ASSET_BACK, ASSET_HOME,
    ASSET_RETRY, ASSET_NEXT, ASSET_PLAY, ASSET_PAUSED, ASSET_SETTINGS,
    ASSET_SOUND_ON, ASSET_SOUND_OFF, ASSET_TICK, ASSET_STAR, ASSET_HINT,
    ASSET_CLOSE, ASSET_LOCK, ASSET_COUNT
};

struct ClickableButton {
    float x, y, w, h;
    int actionCode;
};

class GameMenuStructure {
public:
    GameState currentState = STATE_LOADING;
    int currentPlayingLevel = 1;
    int screenWidth = 0;
    int screenHeight = 0;
    float loadingProgress = 0.0f;
    bool engineInitialized = false;

    // Bound Asset Global References
    jobject assetBitmaps[ASSET_COUNT] = { nullptr };
    std::vector<ClickableButton> UIButtons;

    // Canvas drawing method pointers
    jclass canvasClass = nullptr;
    jclass paintClass = nullptr;
    jclass bitmapClass = nullptr;
    jmethodID midDrawColor = nullptr;
    jmethodID midSave = nullptr;
    jmethodID midTranslate = nullptr;
    jmethodID midScale = nullptr;
    jmethodID midDrawBitmap = nullptr;
    jmethodID midRestore = nullptr;
    jmethodID midDrawText = nullptr;
    jmethodID midGetWidth = nullptr;
    jmethodID midGetHeight = nullptr;

    jobject paintTextReference = nullptr;

    GameMenuStructure() = default;
};

static GameMenuStructure gameUI;

// Helper to draw bitmaps via native layer
void renderBmp(JNIEnv* env, jobject canvas, jobject bitmap, float leftX, float topY, float forcedWidth) {
    if (!canvas || !bitmap || !gameUI.midSave) return;

    int nativeBmpW = 100, nativeBmpH = 100;
    if (gameUI.midGetWidth) nativeBmpW = env->CallIntMethod(bitmap, gameUI.midGetWidth);
    if (gameUI.midGetHeight) nativeBmpH = env->CallIntMethod(bitmap, gameUI.midGetHeight);

    float scaleFactor = forcedWidth / (float)nativeBmpW;

    env->CallIntMethod(canvas, gameUI.midSave);
    env->CallVoidMethod(canvas, gameUI.midTranslate, (jfloat)leftX, (jfloat)topY);
    env->CallVoidMethod(canvas, gameUI.midScale, (jfloat)scaleFactor, (jfloat)scaleFactor, 0.0f, 0.0f);
    env->CallVoidMethod(canvas, gameUI.midDrawBitmap, bitmap, 0.0f, 0.0f, nullptr);
    env->CallVoidMethod(canvas, gameUI.midRestore);
}

extern "C" {

JNIEXPORT void JNICALL
Java_com_night_backgroundchange_MainActivity_initNativeEngine(JNIEnv* env, jobject obj, jboolean dark) {
    gameUI.engineInitialized = false;
    gameUI.currentState = STATE_LOADING;
    gameUI.loadingProgress = 0.0f;

    jclass localCanvas = env->FindClass("android/graphics/Canvas");
    if (localCanvas) gameUI.canvasClass = (jclass)env->NewGlobalRef(localCanvas);
    
    jclass localPaint = env->FindClass("android/graphics/Paint");
    if (localPaint) gameUI.paintClass = (jclass)env->NewGlobalRef(localPaint);

    jclass localBitmap = env->FindClass("android/graphics/Bitmap");
    if (localBitmap) gameUI.bitmapClass = (jclass)env->NewGlobalRef(localBitmap);

    if (gameUI.canvasClass) {
        gameUI.midDrawColor = env->GetMethodID(gameUI.canvasClass, "drawColor", "(I)V");
        gameUI.midSave = env->GetMethodID(gameUI.canvasClass, "save", "()I");
        gameUI.midTranslate = env->GetMethodID(gameUI.canvasClass, "translate", "(FF)V");
        gameUI.midScale = env->GetMethodID(gameUI.canvasClass, "scale", "(FFFF)V");
        gameUI.midDrawBitmap = env->GetMethodID(gameUI.canvasClass, "drawBitmap", "(Landroid/graphics/Bitmap;FFLandroid/graphics/Paint;)V");
        gameUI.midRestore = env->GetMethodID(gameUI.canvasClass, "restore", "()V");
        gameUI.midDrawText = env->GetMethodID(gameUI.canvasClass, "drawText", "(Ljava/lang/String;FFLandroid/graphics/Paint;)V");
    }

    if (gameUI.bitmapClass) {
        gameUI.midGetWidth = env->GetMethodID(gameUI.bitmapClass, "getWidth", "()I");
        gameUI.midGetHeight = env->GetMethodID(gameUI.bitmapClass, "getHeight", "()I");
    }

    if (gameUI.paintClass) {
        jmethodID paintInit = env->GetMethodID(gameUI.paintClass, "<init>", "()V");
        jobject tempPaint = env->NewObject(gameUI.paintClass, paintInit);
        jmethodID setAntiAlias = env->GetMethodID(gameUI.paintClass, "setAntiAlias", "(Z)V");
        jmethodID setColor = env->GetMethodID(gameUI.paintClass, "setColor", "(I)V");
        jmethodID setTextSize = env->GetMethodID(gameUI.paintClass, "setTextSize", "(F)V");
        
        env->CallVoidMethod(tempPaint, setAntiAlias, JNI_TRUE);
        env->CallVoidMethod(tempPaint, setColor, 0xFFFFFFFF); // White UI Text color
        env->CallVoidMethod(tempPaint, setTextSize, 60.0f);
        gameUI.paintTextReference = env->NewGlobalRef(tempPaint);
        env->DeleteLocalRef(tempPaint);
    }

    gameUI.engineInitialized = true;
}

JNIEXPORT void JNICALL
Java_com_night_backgroundchange_MainActivity_nativePushAsset(JNIEnv* env, jobject obj, jint index, jobject bmp) {
    if (index >= 0 && index < ASSET_COUNT && bmp) {
        gameUI.assetBitmaps[index] = env->NewGlobalRef(bmp);
    }
}

JNIEXPORT void JNICALL
Java_com_night_backgroundchange_MainActivity_nativeOnResize(JNIEnv* env, jobject obj, jint w, jint h) {
    gameUI.screenWidth = w;
    gameUI.screenHeight = h;
}

JNIEXPORT void JNICALL
Java_com_night_backgroundchange_MainActivity_nativeRender(JNIEnv* env, jobject obj, jobject canvas) {
    if (!canvas || !gameUI.engineInitialized || !gameUI.midDrawColor) return;

    // Draw dark base background color layout
    env->CallVoidMethod(canvas, gameUI.midDrawColor, 0xFF121212);
    gameUI.UIButtons.clear();

    // --- SCREEN STATE 1: LOADING PAGE ---
    if (gameUI.currentState == STATE_LOADING) {
        gameUI.loadingProgress += 0.02f; // Increment load bar structurally
        if (gameUI.loadingProgress >= 1.0f) {
            gameUI.currentState = STATE_HOME; // Open Home instantly when loaded
        }

        // Render simple Text info tracking
        jstring loadStr = env->NewStringUTF("LOADING LAYOUT...");
        if (gameUI.midDrawText && gameUI.paintTextReference) {
            env->CallVoidMethod(canvas, gameUI.midDrawText, loadStr, (jfloat)(gameUI.screenWidth * 0.25f), (jfloat)(gameUI.screenHeight * 0.5f), gameUI.paintTextReference);
        }
        env->DeleteLocalRef(loadStr);
        return;
    }

    // --- SCREEN STATE 2: HOME SCREEN PAGE ---
    if (gameUI.currentState == STATE_HOME) {
        
        // 1. Head of the game showing the level play number
        std::string lvlText = "LEVEL " + std::to_string(gameUI.currentPlayingLevel);
        jstring jLvlStr = env->NewStringUTF(lvlText.c_str());
        if (gameUI.midDrawText && gameUI.paintTextReference) {
            env->CallVoidMethod(canvas, gameUI.midDrawText, jLvlStr, (jfloat)(gameUI.screenWidth * 0.38f), (jfloat)(gameUI.screenHeight * 0.15f), gameUI.paintTextReference);
        }
        env->DeleteLocalRef(jLvlStr);

        // 2. Play button directly in the center of the game screen
        float playBtnWidth = gameUI.screenWidth * 0.35f;
        float playX = (gameUI.screenWidth / 2.0f) - (playBtnWidth / 2.0f);
        float playY = (gameUI.screenHeight / 2.0f) - (playBtnWidth / 2.0f);
        if (gameUI.assetBitmaps[ASSET_PLAY]) {
            renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_PLAY], playX, playY, playBtnWidth);
            gameUI.UIButtons.push_back({playX, playY, playBtnWidth, playBtnWidth, 1001}); // Action: Play Trigger
        }

        // 3. Settings option button placement
        float settingsWidth = gameUI.screenWidth * 0.15f;
        float setX = 60.0f; // Left margin layout anchor
        float setY = gameUI.screenHeight * 0.82f;
        if (gameUI.assetBitmaps[ASSET_SETTINGS]) {
            renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_SETTINGS], setX, setY, settingsWidth);
            gameUI.UIButtons.push_back({setX, setY, settingsWidth, settingsWidth, 1002}); // Action: Open Settings
        }

        // 4. Bottom right side Home Image occupying approx 10% size allocation
        float homeImageWidth = gameUI.screenWidth * 0.15f; // Approx 10% scaling width bound
        float homeX = gameUI.screenWidth - homeImageWidth - 60.0f; // Bottom right aligned placement
        float homeY = gameUI.screenHeight * 0.82f;
        if (gameUI.assetBitmaps[ASSET_HOME]) {
            renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_HOME], homeX, homeY, homeImageWidth);
            gameUI.UIButtons.push_back({homeX, homeY, homeImageWidth, homeImageWidth, 1003}); // Action: Home Interaction
        }
        return;
    }

    // --- SCREEN STATE 3: SETTINGS OVERLAY PANEL ---
    if (gameUI.currentState == STATE_SETTINGS) {
        jstring setHeader = env->NewStringUTF("SETTINGS MENU");
        if (gameUI.midDrawText && gameUI.paintTextReference) {
            env->CallVoidMethod(canvas, gameUI.midDrawText, setHeader, (jfloat)(gameUI.screenWidth * 0.3f), (jfloat)(gameUI.screenHeight * 0.3f), gameUI.paintTextReference);
        }
        env->DeleteLocalRef(setHeader);

        // Close/Back Button
        float closeW = gameUI.screenWidth * 0.15f;
        float closeX = (gameUI.screenWidth / 2.0f) - (closeW / 2.0f);
        float closeY = gameUI.screenHeight * 0.6f;
        if (gameUI.assetBitmaps[ASSET_CLOSE]) {
            renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_CLOSE], closeX, closeY, closeW);
            gameUI.UIButtons.push_back({closeX, closeY, closeW, closeW, 1004}); // Action: Return to Home
        }
        return;
    }

    // --- SCREEN STATE 4: ACTIVE PLAYING GAMEPLAY STATE ---
    if (gameUI.currentState == STATE_GAMEPLAY) {
        jstring playingText = env->NewStringUTF("GAMEPLAY CONTENT SCREEN");
        if (gameUI.midDrawText && gameUI.paintTextReference) {
            env->CallVoidMethod(canvas, gameUI.midDrawText, playingText, (jfloat)(gameUI.screenWidth * 0.12f), (jfloat)(gameUI.screenHeight * 0.4f), gameUI.paintTextReference);
        }
        env->DeleteLocalRef(playingText);

        // Simple back element option to test navigation layout structure
        float backSize = gameUI.screenWidth * 0.12f;
        if (gameUI.assetBitmaps[ASSET_CLOSE]) {
            renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_CLOSE], 40.0f, 50.0f, backSize);
            gameUI.UIButtons.push_back({40.0f, 50.0f, backSize, backSize, 1004});
        }
    }
}

JNIEXPORT void JNICALL
Java_com_night_backgroundchange_MainActivity_nativeOnTouch(JNIEnv* env, jobject obj, jfloat x, jfloat y) {
    if (!gameUI.engineInitialized) return;

    // Detect structural intercept hit boundaries cleanly
    for (const auto& btn : gameUI.UIButtons) {
        if (x >= btn.x && x <= btn.x + btn.w && y >= btn.y && y <= btn.y + btn.h) {
            if (btn.actionCode == 1001) {
                gameUI.currentState = STATE_GAMEPLAY; // Jump directly to gameplay view structure
            } else if (btn.actionCode == 1002) {
                gameUI.currentState = STATE_SETTINGS; // Open clean placeholder settings layer
            } else if (btn.actionCode == 1003) {
                gameUI.currentState = STATE_HOME;     // Reset view safely back to Home layout state
            } else if (btn.actionCode == 1004) {
                gameUI.currentState = STATE_HOME;     // Back to Home
            }
            break;
        }
    }
}

} // extern "C"
