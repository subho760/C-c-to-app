#include <jni.h>
#include <string>
#include <vector>
#include <android/log.h>

#define LOG_TAG "GameUI"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

enum GameState { STATE_LOADING, STATE_HOME, STATE_SETTINGS, STATE_GAMEPLAY, STATE_LEVELS };
enum ThemeMode { THEME_NORMAL, THEME_BLACK, THEME_SYSTEM };

enum AssetIndex {
    ASSET_ARROW = 0, ASSET_TILE, ASSET_GLOW, ASSET_BACK, ASSET_HOME,
    ASSET_RETRY, ASSET_NEXT, ASSET_PLAY, ASSET_PAUSED, ASSET_SETTINGS,
    ASSET_SOUND_ON, ASSET_SOUND_OFF, ASSET_TICK, ASSET_STAR, ASSET_HINT,
    ASSET_CLOSE, ASSET_LOCK, ASSET_SHARE, ASSET_LEVEL, ASSET_WATCH_ADS, ASSET_COUNT
};

struct ClickableButton {
    float x, y, w, h;
    int actionCode;
    int levelValue;
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

    // Theme states
    ThemeMode activeTheme = THEME_NORMAL;
    bool isCurrentlyDark = false; // Resolved visual mode

    // 20 levels unlock track (Level 1 unlocked by default)
    bool levelsUnlocked[20] = {
        true, false, false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false, false, false
    };

    // Hint popup state
    bool isHintPopupActive = false;

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
    jmethodID midDrawRoundRect = nullptr;

    jobject paintTextReference = nullptr;
    jobject paintShapeReference = nullptr;

    GameMenuStructure() = default;
};

static GameMenuStructure gameUI;

// Utility to render standard bitmaps
void renderBmp(JNIEnv* env, jobject canvas, jobject bitmap, float leftX, float topY, float forcedWidth, jobject customPaint = nullptr) {
    if (!canvas || !bitmap || !gameUI.midSave) return;

    int nativeBmpW = 100, nativeBmpH = 100;
    if (gameUI.midGetWidth) nativeBmpW = env->CallIntMethod(bitmap, gameUI.midGetWidth);
    if (gameUI.midGetHeight) nativeBmpH = env->CallIntMethod(bitmap, gameUI.midGetHeight);

    float scaleFactor = forcedWidth / (float)nativeBmpW;

    env->CallIntMethod(canvas, gameUI.midSave);
    env->CallVoidMethod(canvas, gameUI.midTranslate, (jfloat)leftX, (jfloat)topY);
    env->CallVoidMethod(canvas, gameUI.midScale, (jfloat)scaleFactor, (jfloat)scaleFactor, 0.0f, 0.0f);
    env->CallVoidMethod(canvas, gameUI.midDrawBitmap, bitmap, 0.0f, 0.0f, customPaint);
    env->CallVoidMethod(canvas, gameUI.midRestore);
}

// Draw a round rectangle utilizing SDK paint layer
void drawRoundRectNative(JNIEnv* env, jobject canvas, float left, float top, float right, float bottom, float rx, float ry, int colorHex) {
    if (!canvas || !gameUI.midDrawRoundRect || !gameUI.paintShapeReference) return;
    
    jclass paintCls = env->GetObjectClass(gameUI.paintShapeReference);
    jmethodID setColor = env->GetMethodID(paintCls, "setColor", "(I)V");
    env->CallVoidMethod(gameUI.paintShapeReference, setColor, colorHex);

    // Call dynamic native Canvas.drawRoundRect(float, float, float, float, float, float, Paint)
    env->CallVoidMethod(canvas, gameUI.midDrawRoundRect, left, top, right, bottom, rx, ry, gameUI.paintShapeReference);
}

int getNextUnlockableLevel() {
    for (int i = 0; i < 20; i++) {
        if (!gameUI.levelsUnlocked[i]) return i;
    }
    return -1;
}

extern "C" {

JNIEXPORT void JNICALL
Java_com_night_backgroundchange_MainActivity_initNativeEngine(JNIEnv* env, jobject obj, jboolean systemDark) {
    gameUI.engineInitialized = false;
    gameUI.currentState = STATE_LOADING;
    gameUI.loadingProgress = 0.0f;

    // Dynamic state theme detection
    if (gameUI.activeTheme == THEME_SYSTEM) {
        gameUI.isCurrentlyDark = systemDark;
    } else {
        gameUI.isCurrentlyDark = (gameUI.activeTheme == THEME_BLACK);
    }

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
        gameUI.midDrawRoundRect = env->GetMethodID(gameUI.canvasClass, "drawRoundRect", "(FFFFFFLandroid/graphics/Paint;)V");
    }

    if (gameUI.bitmapClass) {
        gameUI.midGetWidth = env->GetMethodID(gameUI.bitmapClass, "getWidth", "()I");
        gameUI.midGetHeight = env->GetMethodID(gameUI.bitmapClass, "getHeight", "()I");
    }

    // Initialize Global paint setups
    if (gameUI.paintClass) {
        jmethodID paintInit = env->GetMethodID(gameUI.paintClass, "<init>", "()V");
        
        jobject textPaint = env->NewObject(gameUI.paintClass, paintInit);
        jobject shapePaint = env->NewObject(gameUI.paintClass, paintInit);
        
        jmethodID setAntiAlias = env->GetMethodID(gameUI.paintClass, "setAntiAlias", "(Z)V");
        env->CallVoidMethod(textPaint, setAntiAlias, JNI_TRUE);
        env->CallVoidMethod(shapePaint, setAntiAlias, JNI_TRUE);
        
        gameUI.paintTextReference = env->NewGlobalRef(textPaint);
        gameUI.paintShapeReference = env->NewGlobalRef(shapePaint);

        env->DeleteLocalRef(textPaint);
        env->DeleteLocalRef(shapePaint);
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

// Access helper to obtain color filter reference tints from Java View layer
jobject getTintPaint(JNIEnv* env, jobject obj, int colorHex) {
    jclass actCls = env->GetObjectClass(obj);
    jmethodID getTintMid = env->GetMethodID(actCls, "getTintedPaint", "(I)Landroid/graphics/Paint;");
    if (getTintMid) {
        return env->CallObjectMethod(obj, getTintMid, colorHex);
    }
    return nullptr;
}

JNIEXPORT void JNICALL
Java_com_night_backgroundchange_MainActivity_nativeRender(JNIEnv* env, jobject obj, jobject canvas) {
    if (!canvas || !gameUI.engineInitialized || !gameUI.midDrawColor) return;

    // Theme Color Swaps
    int baseBgColor = gameUI.isCurrentlyDark ? 0xFF121212 : 0xFFFFFFFF;
    int baseTxtColor = gameUI.isCurrentlyDark ? 0xFFFFFFFF : 0xFF000000;
    int primaryTint = gameUI.isCurrentlyDark ? 0xFFFFFFFF : 0xFF000000;
    int lightGray = 0xFFD3D3D3;

    env->CallVoidMethod(canvas, gameUI.midDrawColor, baseBgColor);
    gameUI.UIButtons.clear();

    // Set Text color
    if (gameUI.paintTextReference) {
        jclass paintCls = env->GetObjectClass(gameUI.paintTextReference);
        jmethodID setColor = env->GetMethodID(paintCls, "setColor", "(I)V");
        jmethodID setTextSize = env->GetMethodID(paintCls, "setTextSize", "(F)V");
        env->CallVoidMethod(gameUI.paintTextReference, setColor, baseTxtColor);
        env->CallVoidMethod(gameUI.paintTextReference, setTextSize, 45.0f);
    }

    jobject tintActive = getTintPaint(env, obj, primaryTint);
    jobject tintGray = getTintPaint(env, obj, lightGray);

    // --- STATE 1: LOADING PAGE ---
    if (gameUI.currentState == STATE_LOADING) {
        gameUI.loadingProgress += 0.02f;
        if (gameUI.loadingProgress >= 1.0f) {
            gameUI.currentState = STATE_HOME;
        }
        jstring loadStr = env->NewStringUTF("LOADING RUN ARROW...");
        env->CallVoidMethod(canvas, gameUI.midDrawText, loadStr, (jfloat)(gameUI.screenWidth * 0.25f), (jfloat)(gameUI.screenHeight * 0.5f), gameUI.paintTextReference);
        env->DeleteLocalRef(loadStr);
        return;
    }

    // --- RENDER REUSABLE NAV FOOTER (Shared between Home, Settings, Levels) ---
    float footerHeight = gameUI.screenHeight * 0.12f;
    float footerY = gameUI.screenHeight - footerHeight;
    
    if (gameUI.currentState == STATE_HOME || gameUI.currentState == STATE_SETTINGS || gameUI.currentState == STATE_LEVELS) {
        // Draw Footer Base Line/Background divider
        drawRoundRectNative(env, canvas, 0, footerY, gameUI.screenWidth, gameUI.screenHeight, 0, 0, gameUI.isCurrentlyDark ? 0xFF1E1E1E : 0xFFF5F5F5);

        float iconSize = gameUI.screenWidth * 0.08f;
        float itemSpacing = gameUI.screenWidth / 4.0f;
        float centerY = footerY + (footerHeight / 2.0f) - (iconSize / 2.0f);

        // Footer Item 1: Home
        bool isHomeActive = (gameUI.currentState == STATE_HOME);
        float homeX = itemSpacing - (iconSize / 2.0f);
        jobject homePaint = isHomeActive ? tintActive : tintGray;
        if (gameUI.assetBitmaps[ASSET_HOME]) {
            renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_HOME], homeX, centerY, iconSize, homePaint);
            gameUI.UIButtons.push_back({homeX - 20, centerY - 20, iconSize + 40, iconSize + 40, 9001, 0});
        }

        // Footer Item 2: Levels Selection (Centered)
        bool isLevelsActive = (gameUI.currentState == STATE_LEVELS);
        float lvlX = (itemSpacing * 2.0f) - (iconSize / 2.0f);
        jobject lvlPaint = isLevelsActive ? tintActive : tintGray;
        if (gameUI.assetBitmaps[ASSET_LEVEL]) {
            renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_LEVEL], lvlX, centerY, iconSize, lvlPaint);
            gameUI.UIButtons.push_back({lvlX - 20, centerY - 20, iconSize + 40, iconSize + 40, 9002, 0});
        }

        // Footer Item 3: Settings
        bool isSettingsActive = (gameUI.currentState == STATE_SETTINGS);
        float setX = (itemSpacing * 3.0f) - (iconSize / 2.0f);
        jobject setPaint = isSettingsActive ? tintActive : tintGray;
        if (gameUI.assetBitmaps[ASSET_SETTINGS]) {
            renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_SETTINGS], setX, centerY, iconSize, setPaint);
            gameUI.UIButtons.push_back({setX - 20, centerY - 20, iconSize + 40, iconSize + 40, 9003, 0});
        }
    }

    // --- STATE 2: HOME SCREEN PAGE ---
    if (gameUI.currentState == STATE_HOME) {
        // App title Header
        jstring titleStr = env->NewStringUTF("RUN ARROW");
        if (gameUI.paintTextReference) {
            jclass paintCls = env->GetObjectClass(gameUI.paintTextReference);
            jmethodID setTextSize = env->GetMethodID(paintCls, "setTextSize", "(F)V");
            env->CallVoidMethod(gameUI.paintTextReference, setTextSize, 80.0f);
        }
        env->CallVoidMethod(canvas, gameUI.midDrawText, titleStr, (jfloat)(gameUI.screenWidth * 0.28f), (jfloat)(gameUI.screenHeight * 0.2f), gameUI.paintTextReference);
        env->DeleteLocalRef(titleStr);

        // Restore Text standard size
        if (gameUI.paintTextReference) {
            jclass paintCls = env->GetObjectClass(gameUI.paintTextReference);
            jmethodID setTextSize = env->GetMethodID(paintCls, "setTextSize", "(F)V");
            env->CallVoidMethod(gameUI.paintTextReference, setTextSize, 45.0f);
        }

        // Centered Play Button
        float playW = gameUI.screenWidth * 0.32f;
        float playX = (gameUI.screenWidth / 2.0f) - (playW / 2.0f);
        float playY = (gameUI.screenHeight * 0.38f);
        if (gameUI.assetBitmaps[ASSET_PLAY]) {
            renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_PLAY], playX, playY, playW, tintActive);
            gameUI.UIButtons.push_back({playX, playY, playW, playW, 2001, 0});
        }

        // Level information text displayed right below Play Button
        std::string infoStr = "PLAYING LEVEL " + std::to_string(gameUI.currentPlayingLevel);
        jstring jInfo = env->NewStringUTF(infoStr.c_str());
        env->CallVoidMethod(canvas, gameUI.midDrawText, jInfo, (jfloat)(gameUI.screenWidth * 0.33f), (jfloat)(gameUI.screenHeight * 0.65f), gameUI.paintTextReference);
        env->DeleteLocalRef(jInfo);
    }

    // --- STATE 3: LEVELS SELECTION MATRIX (20 Levels) ---
    if (gameUI.currentState == STATE_LEVELS) {
        jstring levelHeader = env->NewStringUTF("SELECT LEVEL");
        env->CallVoidMethod(canvas, gameUI.midDrawText, levelHeader, (jfloat)(gameUI.screenWidth * 0.32f), (jfloat)(gameUI.screenHeight * 0.08f), gameUI.paintTextReference);
        env->DeleteLocalRef(levelHeader);

        float sizeBox = gameUI.screenWidth * 0.16f;
        float spaceX = gameUI.screenWidth * 0.04f;
        float startGridX = (gameUI.screenWidth - (4 * sizeBox + 3 * spaceX)) / 2.0f;
        float startGridY = gameUI.screenHeight * 0.13f;

        int nextUnlockIndex = getNextUnlockableLevel();

        for (int i = 0; i < 20; i++) {
            int row = i / 4;
            int col = i % 4;
            float bx = startGridX + col * (sizeBox + spaceX);
            float by = startGridY + row * (sizeBox + spaceX);

            bool isUnlocked = gameUI.levelsUnlocked[i];
            bool isAdUnlockable = (i == nextUnlockIndex);

            // Level Box Draw with rounded radius
            int boxColor = isUnlocked ? 0xFF007AFF : 0xFFE0E0E0; // Blue if unlocked, gray if locked
            drawRoundRectNative(env, canvas, bx, by, bx + sizeBox, by + sizeBox, 16, 16, boxColor);

            // Watermark lock symbol drawing on locked boxes
            if (!isUnlocked) {
                if (gameUI.assetBitmaps[ASSET_LOCK]) {
                    renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_LOCK], bx + (sizeBox * 0.25f), by + (sizeBox * 0.25f), sizeBox * 0.5f, tintActive);
                }
                if (isAdUnlockable) {
                    if (gameUI.assetBitmaps[ASSET_WATCH_ADS]) {
                        renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_WATCH_ADS], bx + (sizeBox * 0.6f), by + 5.0f, sizeBox * 0.35f);
                    }
                }
            } else {
                std::string lvlNumStr = std::to_string(i + 1);
                jstring jLvlNum = env->NewStringUTF(lvlNumStr.c_str());
                env->CallVoidMethod(canvas, gameUI.midDrawText, jLvlNum, bx + (sizeBox * 0.35f), by + (sizeBox * 0.62f), gameUI.paintTextReference);
                env->DeleteLocalRef(jLvlNum);
            }

            gameUI.UIButtons.push_back({bx, by, sizeBox, sizeBox, 3000 + i, i});
        }
    }

    // --- STATE 4: SETTINGS PANEL ---
    if (gameUI.currentState == STATE_SETTINGS) {
        float optionY = gameUI.screenHeight * 0.15f;
        float optionHeight = 120.0f;
        float optionWidth = gameUI.screenWidth * 0.85f;
        float optionX = (gameUI.screenWidth - optionWidth) / 2.0f;

        // Draw Options list items
        std::string themes[] = {"1. THEME: NORMAL", "1. THEME: BLACK", "1. THEME: SYSTEM"};
        jstring themeText = env->NewStringUTF(themes[gameUI.activeTheme].c_str());
        env->CallVoidMethod(canvas, gameUI.midDrawText, themeText, optionX + 20, optionY + 70, gameUI.paintTextReference);
        env->DeleteLocalRef(themeText);
        gameUI.UIButtons.push_back({optionX, optionY, optionWidth, optionHeight, 6001, 0});

        jstring privText = env->NewStringUTF("2. PRIVACY POLICY");
        env->CallVoidMethod(canvas, gameUI.midDrawText, privText, optionX + 20, optionY + 210, gameUI.paintTextReference);
        env->DeleteLocalRef(privText);
        gameUI.UIButtons.push_back({optionX, optionY + 140, optionWidth, optionHeight, 6002, 0});

        jstring shareText = env->NewStringUTF("3. SHARE OUR APP");
        env->CallVoidMethod(canvas, gameUI.midDrawText, shareText, optionX + 20, optionY + 350, gameUI.paintTextReference);
        env->DeleteLocalRef(shareText);
        if (gameUI.assetBitmaps[ASSET_SHARE]) {
            renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_SHARE], optionX + optionWidth - 80, optionY + 290, 60.0f, tintActive);
        }
        gameUI.UIButtons.push_back({optionX, optionY + 280, optionWidth, optionHeight, 6003, 0});

        jstring rateText = env->NewStringUTF("4. RATE OUR APP");
        env->CallVoidMethod(canvas, gameUI.midDrawText, rateText, optionX + 20, optionY + 490, gameUI.paintTextReference);
        env->DeleteLocalRef(rateText);
        if (gameUI.assetBitmaps[ASSET_STAR]) {
            renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_STAR], optionX + optionWidth - 80, optionY + 430, 60.0f, tintActive);
        }
        gameUI.UIButtons.push_back({optionX, optionY + 420, optionWidth, optionHeight, 6004, 0});
    }

    // --- STATE 5: ACTIVE PLAYING GAMEPLAY SCREEN ---
    if (gameUI.currentState == STATE_GAMEPLAY) {
        // Pause Button on the LEFT side of top header
        float pauseBtnSize = gameUI.screenWidth * 0.10f;
        float pauseX = 40.0f;
        float pauseY = 50.0f;
        if (gameUI.assetBitmaps[ASSET_PAUSED]) {
            renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_PAUSED], pauseX, pauseY, pauseBtnSize, tintGray);
            gameUI.UIButtons.push_back({pauseX, pauseY, pauseBtnSize, pauseBtnSize, 4002, 0});
        }

        // Hint Button on the RIGHT side of top header (Yellow Tinted)
        float hintX = gameUI.screenWidth - pauseBtnSize - 40.0f;
        jobject yellowTint = getTintPaint(env, obj, 0xFFFFCC00);
        if (gameUI.assetBitmaps[ASSET_HINT]) {
            renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_HINT], hintX, pauseY, pauseBtnSize, yellowTint);
            gameUI.UIButtons.push_back({hintX, pauseY, pauseBtnSize, pauseBtnSize, 4003, 0});
        }

        // Level Header text
        std::string playHeader = "LEVEL " + std::to_string(gameUI.currentPlayingLevel);
        jstring jPlayHead = env->NewStringUTF(playHeader.c_str());
        env->CallVoidMethod(canvas, gameUI.midDrawText, jPlayHead, (jfloat)(gameUI.screenWidth * 0.38f), 90.0f, gameUI.paintTextReference);
        env->DeleteLocalRef(jPlayHead);

        // Win Level Trigger button (simulation helper to easily test level progression)
        jstring clearTxt = env->NewStringUTF("[ Tap Here to Clear Level ]");
        env->CallVoidMethod(canvas, gameUI.midDrawText, clearTxt, (jfloat)(gameUI.screenWidth * 0.15f), (jfloat)(gameUI.screenHeight * 0.5f), gameUI.paintTextReference);
        env->DeleteLocalRef(clearTxt);
        gameUI.UIButtons.push_back({(float)(gameUI.screenWidth * 0.15f), (float)(gameUI.screenHeight * 0.45f), (float)(gameUI.screenWidth * 0.7f), 80.0f, 4001, 0});

        // --- HINT DIALOG POPUP (40% Blurred background overlay) ---
         if (gameUI.isHintPopupActive) {
            // Draw blur translucent gray scrim block over screen surface
            drawRoundRectNative(env, canvas, 0, 0, gameUI.screenWidth, gameUI.screenHeight, 0, 0, 0x66000000); // 40% opaque dark mask

            // White dialog layout center screen
            float popW = gameUI.screenWidth * 0.8f;
            float popH = gameUI.screenHeight * 0.35f;
            float popX = (gameUI.screenWidth - popW) / 2.0f;
            float popY = (gameUI.screenHeight - popH) / 2.0f;

            drawRoundRectNative(env, canvas, popX, popY, popX + popW, popY + popH, 32, 32, gameUI.isCurrentlyDark ? 0xFF1E1E1E : 0xFFFFFFFF);

            // Message text "need hint?"
            jstring msg = env->NewStringUTF("Need hint?");
            env->CallVoidMethod(canvas, gameUI.midDrawText, msg, popX + 60.0f, popY + 120.0f, gameUI.paintTextReference);
            env->DeleteLocalRef(msg);

            // Close button top-right of dialog
            float closeSize = 50.0f;
            float closeX = popX + popW - closeSize - 40.0f;
            float closeY = popY + 40.0f;
            if (gameUI.assetBitmaps[ASSET_CLOSE]) {
                renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_CLOSE], closeX, closeY, closeSize, tintActive);
                gameUI.UIButtons.push_back({closeX - 10, closeY - 10, closeSize + 20, closeSize + 20, 8001, 0}); // Close Popup
            }

            // Blue Watch Ads Button
            float adBtnW = popW * 0.75f;
            float adBtnH = 90.0f;
            float adBtnX = popX + (popW - adBtnW) / 2.0f;
            float adBtnY = popY + popH - adBtnH - 50.0f;

            drawRoundRectNative(env, canvas, adBtnX, adBtnY, adBtnX + adBtnW, adBtnY + adBtnH, 16, 16, 0xFF007AFF);

            // Draw watchads.png and Text inside the button
            if (gameUI.assetBitmaps[ASSET_WATCH_ADS]) {
                renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_WATCH_ADS], adBtnX + 30.0f, adBtnY + 20.0f, 50.0f);
            }
            jstring adBtnTxt = env->NewStringUTF("Watch Ads");
            env->CallVoidMethod(canvas, gameUI.midDrawText, adBtnTxt, adBtnX + 110.0f, adBtnY + 60.0f, gameUI.paintTextReference);
            env->DeleteLocalRef(adBtnTxt);

            gameUI.UIButtons.push_back({adBtnX, adBtnY, adBtnW, adBtnH, 8002, 0});
        }
    }
}

JNIEXPORT void JNICALL
Java_com_night_backgroundchange_MainActivity_nativeOnTouch(JNIEnv* env, jobject obj, jfloat x, jfloat y) {
    if (!gameUI.engineInitialized) return;

    // Filter touch intercepts if Hint modal is active
    if (gameUI.isHintPopupActive) {
        for (const auto& btn : gameUI.UIButtons) {
            if (x >= btn.x && x <= btn.x + btn.w && y >= btn.y && y <= btn.y + btn.h) {
                if (btn.actionCode == 8001) { // Close
                    gameUI.isHintPopupActive = false;
                } else if (btn.actionCode == 8002) { // Play Ad
                    gameUI.isHintPopupActive = false; // Add Ad SDK triggers here
                }
                return;
            }
        }
        return; // Absorb all outside background touch events
    }

    for (const auto& btn : gameUI.UIButtons) {
        if (x >= btn.x && x <= btn.x + btn.w && y >= btn.y && y <= btn.y + btn.h) {
            
            // --- Handlers for 20-Levels touch matrix ---
            if (btn.actionCode >= 3000 && btn.actionCode <= 3019) {
                int selectedLevelIndex = btn.levelValue;
                bool isUnlocked = gameUI.levelsUnlocked[selectedLevelIndex];
                int nextUnlock = getNextUnlockableLevel();

                if (isUnlocked) {
                    gameUI.currentPlayingLevel = selectedLevelIndex + 1;
                    gameUI.currentState = STATE_GAMEPLAY;
                } else if (selectedLevelIndex == nextUnlock) {
                    // Unlock next level directly via watcher
                    gameUI.levelsUnlocked[selectedLevelIndex] = true; 
                    gameUI.currentPlayingLevel = selectedLevelIndex + 1;
                    gameUI.currentState = STATE_GAMEPLAY;
                }
                break;
            }

            switch (btn.actionCode) {
                case 9001: // Footer Home
                    gameUI.currentState = STATE_HOME;
                    break;
                case 9002: // Footer Levels
                    gameUI.currentState = STATE_LEVELS;
                    break;
                case 9003: // Footer Settings
                    gameUI.currentState = STATE_SETTINGS;
                    break;
                case 2001: // Home play trigger
                    gameUI.currentState = STATE_GAMEPLAY;
                    break;
                case 4001: // Clear Level
                    if (gameUI.currentPlayingLevel < 20) {
                        gameUI.levelsUnlocked[gameUI.currentPlayingLevel] = true;
                        gameUI.currentPlayingLevel++;
                    }
                    gameUI.currentState = STATE_LEVELS;
                    break;
                case 4002: // Open pause overlay state
                    gameUI.currentState = STATE_HOME; // Returns home directly as requested
                    break;
                case 4003: // Open Hint Pop-up modal
                    gameUI.isHintPopupActive = true;
                    break;
                case 6001: { // Rotate Themes
                    int curTheme = (int)gameUI.activeTheme;
                    curTheme = (curTheme + 1) % 3;
                    gameUI.activeTheme = (ThemeMode)curTheme;
                    
                    // Force refresh theme calculation mapping
                    jclass actCls = env->GetObjectClass(obj);
                    jmethodID refreshMid = env->GetMethodID(actCls, "triggerThemeRebuild", "()V");
                    if (refreshMid) env->CallVoidMethod(obj, refreshMid);
                    break;
                }
            }
            break;
        }
    }
}

} // extern "C"
