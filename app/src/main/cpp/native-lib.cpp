#include <jni.h>
#include <string>
#include <vector>
#include <android/log.h>

#define LOG_TAG "GameUI"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

enum GameState { STATE_LOADING, STATE_HOME, STATE_SETTINGS, STATE_GAMEPLAY, STATE_PAUSED };

enum AssetIndex {
    ASSET_ARROW = 0, ASSET_TILE, ASSET_GLOW, ASSET_BACK, ASSET_HOME,
    ASSET_RETRY, ASSET_NEXT, ASSET_PLAY, ASSET_PAUSED, ASSET_SETTINGS,
    ASSET_SOUND_ON, ASSET_SOUND_OFF, ASSET_TICK, ASSET_STAR, ASSET_HINT,
    ASSET_CLOSE, ASSET_LOCK, ASSET_COUNT
};

struct ClickableButton {
    float x, y, w, h;
    int actionCode; // Unique action ID
    int levelValue; // Linked level if applicable
};

class GameMenuStructure {
public:
    GameState currentState = STATE_LOADING;
    int currentPlayingLevel = 1;
    int screenWidth = 0;
    int screenHeight = 0;
    float loadingProgress = 0.0f;
    bool engineInitialized = false;
    bool audioEnabled = true;

    // Level unlock states (Level 1 is unlocked by default)
    bool levelsUnlocked[10] = {true, false, false, false, false, false, false, false, false, false};

    jobject assetBitmaps[ASSET_COUNT] = { nullptr };
    std::vector<ClickableButton> UIButtons;

    // JNI Canvas drawing helpers
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

// Find which level is the exact next locked level
int getNextUnlockableLevel() {
    for (int i = 0; i < 10; i++) {
        if (!gameUI.levelsUnlocked[i]) {
            return i; // Index of the next level to unlock
        }
    }
    return -1;
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
        env->CallVoidMethod(tempPaint, setColor, 0xFF000000); // Black UI Text color
        env->CallVoidMethod(tempPaint, setTextSize, 45.0f);
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

    // Set White background color
    env->CallVoidMethod(canvas, gameUI.midDrawColor, 0xFFFFFFFF);
    gameUI.UIButtons.clear();

    // --- STATE 1: LOADING PAGE ---
    if (gameUI.currentState == STATE_LOADING) {
        gameUI.loadingProgress += 0.02f;
        if (gameUI.loadingProgress >= 1.0f) {
            gameUI.currentState = STATE_HOME;
        }

        jstring loadStr = env->NewStringUTF("LOADING ASSETS...");
        if (gameUI.midDrawText && gameUI.paintTextReference) {
            env->CallVoidMethod(canvas, gameUI.midDrawText, loadStr, (jfloat)(gameUI.screenWidth * 0.25f), (jfloat)(gameUI.screenHeight * 0.5f), gameUI.paintTextReference);
        }
        env->DeleteLocalRef(loadStr);
        return;
    }

    // --- STATE 2: HOME SCREEN ---
    if (gameUI.currentState == STATE_HOME) {
        // Play text/header layout
        std::string header = "PLAY LEVEL " + std::to_string(gameUI.currentPlayingLevel);
        jstring jHeader = env->NewStringUTF(header.c_str());
        if (gameUI.midDrawText && gameUI.paintTextReference) {
            env->CallVoidMethod(canvas, gameUI.midDrawText, jHeader, (jfloat)(gameUI.screenWidth * 0.32f), (jfloat)(gameUI.screenHeight * 0.1f), gameUI.paintTextReference);
        }
        env->DeleteLocalRef(jHeader);

        // Play Button in the Center
        float playBtnWidth = gameUI.screenWidth * 0.32f;
        float playX = (gameUI.screenWidth / 2.0f) - (playBtnWidth / 2.0f);
        float playY = (gameUI.screenHeight * 0.22f);
        if (gameUI.assetBitmaps[ASSET_PLAY]) {
            renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_PLAY], playX, playY, playBtnWidth);
            gameUI.UIButtons.push_back({playX, playY, playBtnWidth, playBtnWidth, 2001, 0}); // Action Code: Start Gameplay
        }

        // --- THE 10 LEVELS OPTION MATRIX (Middle-Bottom Section) ---
        float gridY = gameUI.screenHeight * 0.45f;
        float btnSize = gameUI.screenWidth * 0.14f;
        float spacingX = gameUI.screenWidth * 0.04f;
        float startX = (gameUI.screenWidth - (5 * btnSize + 4 * spacingX)) / 2.0f;

        int nextUnlockableIndex = getNextUnlockableLevel();

        for (int i = 0; i < 10; i++) {
            int row = i / 5;
            int col = i % 5;
            float bx = startX + col * (btnSize + spacingX);
            float by = gridY + row * (btnSize + spacingX);

            bool isUnlocked = gameUI.levelsUnlocked[i];
            bool isAdUnlockable = (i == nextUnlockableIndex); // Only the next locked level can be unlocked with ads

            // Store Touch button bounds
            gameUI.UIButtons.push_back({bx, by, btnSize, btnSize, 3000 + i, i});

            // Draw Level box background border / fill
            if (isUnlocked) {
                // Draw normal unlocked button (draw a small indicator or plain number text)
                std::string lNum = std::to_string(i + 1);
                jstring jlNum = env->NewStringUTF(lNum.c_str());
                env->CallVoidMethod(canvas, gameUI.midDrawText, jlNum, bx + (btnSize * 0.3f), by + (btnSize * 0.65f), gameUI.paintTextReference);
                env->DeleteLocalRef(jlNum);
            } else {
                // If locked, draw the lock asset
                if (gameUI.assetBitmaps[ASSET_LOCK]) {
                    renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_LOCK], bx, by, btnSize);
                }
                // If this is the immediate next locked level, draw a Watch Ad helper tag
                if (isAdUnlockable) {
                    jstring adTag = env->NewStringUTF("AD");
                    env->CallVoidMethod(canvas, gameUI.midDrawText, adTag, bx + (btnSize * 0.15f), by - 10.0f, gameUI.paintTextReference);
                    env->DeleteLocalRef(adTag);
                }
            }
        }

        // --- BOTTOM ROW: HOME (LEFT) AND SETTING (RIGHT) ---
        // Home and Settings are scaled 40% smaller (Width multiplied by 0.6)
        float bottomIconSize = (gameUI.screenWidth * 0.15f) * 0.60f; 
        float bottomY = gameUI.screenHeight * 0.82f;

        // Home in the Left position
        float homeX = 60.0f; 
        if (gameUI.assetBitmaps[ASSET_HOME]) {
            renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_HOME], homeX, bottomY, bottomIconSize);
            gameUI.UIButtons.push_back({homeX, bottomY, bottomIconSize, bottomIconSize, 2002, 0}); // Go Home Action
        }

        // Setting in the Right position
        float setX = gameUI.screenWidth - bottomIconSize - 60.0f;
        if (gameUI.assetBitmaps[ASSET_SETTINGS]) {
            renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_SETTINGS], setX, bottomY, bottomIconSize);
            gameUI.UIButtons.push_back({setX, bottomY, bottomIconSize, bottomIconSize, 2003, 0}); // Settings Action
        }
        return;
    }

    // --- STATE 3: SETTINGS VIEW ---
    if (gameUI.currentState == STATE_SETTINGS) {
        jstring setHeader = env->NewStringUTF("SETTINGS");
        if (gameUI.midDrawText && gameUI.paintTextReference) {
            env->CallVoidMethod(canvas, gameUI.midDrawText, setHeader, (jfloat)(gameUI.screenWidth * 0.35f), (jfloat)(gameUI.screenHeight * 0.3f), gameUI.paintTextReference);
        }
        env->DeleteLocalRef(setHeader);

        // Simple Close / Back button
        float closeW = gameUI.screenWidth * 0.15f;
        float closeX = (gameUI.screenWidth / 2.0f) - (closeW / 2.0f);
        float closeY = gameUI.screenHeight * 0.6f;
        if (gameUI.assetBitmaps[ASSET_CLOSE]) {
            renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_CLOSE], closeX, closeY, closeW);
            gameUI.UIButtons.push_back({closeX, closeY, closeW, closeW, 2002, 0}); // Back to Home
        }
        return;
    }

    // --- STATE 4: ACTIVE GAMEPLAY ---
    if (gameUI.currentState == STATE_GAMEPLAY) {
        std::string gameInfo = "PLAYING LEVEL " + std::to_string(gameUI.currentPlayingLevel);
        jstring jGameInfo = env->NewStringUTF(gameInfo.c_str());
        if (gameUI.midDrawText && gameUI.paintTextReference) {
            env->CallVoidMethod(canvas, gameUI.midDrawText, jGameInfo, (jfloat)(gameUI.screenWidth * 0.25f), (jfloat)(gameUI.screenHeight * 0.4f), gameUI.paintTextReference);
        }
        env->DeleteLocalRef(jGameInfo);

        // Win Level Trigger button (simulation helper to easily test level progression)
        jstring clearTxt = env->NewStringUTF("[ Tap Here to Clear Level ]");
        env->CallVoidMethod(canvas, gameUI.midDrawText, clearTxt, (jfloat)(gameUI.screenWidth * 0.15f), (jfloat)(gameUI.screenHeight * 0.6f), gameUI.paintTextReference);
        env->DeleteLocalRef(clearTxt);
        gameUI.UIButtons.push_back({(float)(gameUI.screenWidth * 0.15f), (float)(gameUI.screenHeight * 0.55f), (float)(gameUI.screenWidth * 0.7f), 80.0f, 4001, 0});

        // Pause Button on the Right side of the screen
        float pauseBtnSize = gameUI.screenWidth * 0.12f;
        float pauseX = gameUI.screenWidth - pauseBtnSize - 40.0f;
        float pauseY = 50.0f;
        if (gameUI.assetBitmaps[ASSET_PAUSED]) {
            renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_PAUSED], pauseX, pauseY, pauseBtnSize);
            gameUI.UIButtons.push_back({pauseX, pauseY, pauseBtnSize, pauseBtnSize, 4002, 0}); // Action: Show Pause Overlay
        }
        return;
    }

    // --- STATE 5: PAUSED MENU OVERLAY ---
    if (gameUI.currentState == STATE_PAUSED) {
        // Draw Pause Header Text
        jstring pauseTitle = env->NewStringUTF("GAME PAUSED");
        env->CallVoidMethod(canvas, gameUI.midDrawText, pauseTitle, (jfloat)(gameUI.screenWidth * 0.32f), (jfloat)(gameUI.screenHeight * 0.25f), gameUI.paintTextReference);
        env->DeleteLocalRef(pauseTitle);

        float itemH = 100.0f;
        float startY = gameUI.screenHeight * 0.38f;
        float itemW = gameUI.screenWidth * 0.6f;
        float itemX = (gameUI.screenWidth - itemW) / 2.0f;

        // 1. Resume / Play Button option
        jstring playTxt = env->NewStringUTF("1. RESUME GAME");
        env->CallVoidMethod(canvas, gameUI.midDrawText, playTxt, itemX, startY, gameUI.paintTextReference);
        env->DeleteLocalRef(playTxt);
        gameUI.UIButtons.push_back({itemX, startY - 60.0f, itemW, itemH, 5001, 0});

        // 2. Retry Button option
        jstring retryTxt = env->NewStringUTF("2. RETRY LEVEL");
        env->CallVoidMethod(canvas, gameUI.midDrawText, retryTxt, itemX, startY + 120.0f, gameUI.paintTextReference);
        env->DeleteLocalRef(retryTxt);
        gameUI.UIButtons.push_back({itemX, startY + 60.0f, itemW, itemH, 5002, 0});

        // 3. Dynamic Audio On / Audio Off Toggle Option
        std::string audioStr = gameUI.audioEnabled ? "3. AUDIO ON" : "3. AUDIO OFF";
        jstring jAudio = env->NewStringUTF(audioStr.c_str());
        env->CallVoidMethod(canvas, gameUI.midDrawText, jAudio, itemX, startY + 240.0f, gameUI.paintTextReference);
        env->DeleteLocalRef(jAudio);
        gameUI.UIButtons.push_back({itemX, startY + 180.0f, itemW, itemH, 5003, 0});

        // 4. Back to Home Option (Extra helpful option)
        jstring homeExit = env->NewStringUTF("4. QUIT TO MENU");
        env->CallVoidMethod(canvas, gameUI.midDrawText, homeExit, itemX, startY + 360.0f, gameUI.paintTextReference);
        env->DeleteLocalRef(homeExit);
        gameUI.UIButtons.push_back({itemX, startY + 300.0f, itemW, itemH, 2002, 0});
    }
}

JNIEXPORT void JNICALL
Java_com_night_backgroundchange_MainActivity_nativeOnTouch(JNIEnv* env, jobject obj, jfloat x, jfloat y) {
    if (!gameUI.engineInitialized) return;

    for (const auto& btn : gameUI.UIButtons) {
        if (x >= btn.x && x <= btn.x + btn.w && y >= btn.y && y <= btn.y + btn.h) {
            
            // --- Handlers for 10-Levels Matrix touch interactions ---
            if (btn.actionCode >= 3000 && btn.actionCode <= 3009) {
                int selectedLevelIndex = btn.levelValue;
                bool isUnlocked = gameUI.levelsUnlocked[selectedLevelIndex];
                int nextUnlockable = getNextUnlockableLevel();

                if (isUnlocked) {
                    gameUI.currentPlayingLevel = selectedLevelIndex + 1;
                    gameUI.currentState = STATE_GAMEPLAY;
                } else if (selectedLevelIndex == nextUnlockable) {
                    // This is the immediate next level, let them watch an Ad to unlock it
                    gameUI.levelsUnlocked[selectedLevelIndex] = true; 
                    gameUI.currentPlayingLevel = selectedLevelIndex + 1;
                    gameUI.currentState = STATE_GAMEPLAY;
                }
                break;
            }

            switch (btn.actionCode) {
                case 2001: // Centered Play Button
                    gameUI.currentState = STATE_GAMEPLAY;
                    break;
                case 2002: // Home Button
                    gameUI.currentState = STATE_HOME;
                    break;
                case 2003: // Settings Button
                    gameUI.currentState = STATE_SETTINGS;
                    break;
                case 4001: // Simulation: Win/Clear Current Level
                    if (gameUI.currentPlayingLevel < 10) {
                        gameUI.levelsUnlocked[gameUI.currentPlayingLevel] = true; // Unlock the next one
                        gameUI.currentPlayingLevel++;
                    }
                    gameUI.currentState = STATE_HOME;
                    break;
                case 4002: // Pause Action Button (triggers overlay state)
                    gameUI.currentState = STATE_PAUSED;
                    break;
                case 5001: // Pause Option 1: Resume
                    gameUI.currentState = STATE_GAMEPLAY;
                    break;
                case 5002: // Pause Option 2: Retry
                    gameUI.currentState = STATE_GAMEPLAY; // Reloads current level UI clean
                    break;
                case 5003: // Pause Option 3: Sound Toggle
                    gameUI.audioEnabled = !gameUI.audioEnabled;
                    break;
            }
            break;
        }
    }
}

} // extern "C"
