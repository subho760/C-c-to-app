#include <jni.h>
#include <string>
#include <vector>
#include <android/log.h>

#define LOG_TAG "GameEngine"
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

    // Theme Management
    ThemeMode activeTheme = THEME_NORMAL;
    bool isCurrentlyDark = false;

    // 50 Levels Management array
    bool levelsUnlocked[50] = { true, false }; // Level 1 is unlocked initially

    // Active Popups Configuration Management
    bool isHintPopupActive = false;
    bool isThemePopupActive = false;
    bool isRatingPopupActive = false;
    bool isPausePopupActive = false;

    // Rating Dialog State Trackers
    int selectedRatingStars = 0;

    // JNI Resource Handles
    jobject assetBitmaps[ASSET_COUNT] = { nullptr };
    std::vector<ClickableButton> UIButtons;

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
    jmethodID midDrawRoundRect = nullptr;
    jmethodID midDrawLine = nullptr;
    jmethodID midGetWidth = nullptr;
    jmethodID midGetHeight = nullptr;

    jobject paintTextReference = nullptr;
    jobject paintShapeReference = nullptr;

    GameMenuStructure() = default;
};

static GameMenuStructure gameUI;

// Structural bitmap drawing utility using the graphics layer
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

// Renders soft shadows underneath actionable canvas containers
void drawRealShadowRoundRect(JNIEnv* env, jobject canvas, float left, float top, float right, float bottom, float rx, float ry) {
    if (!canvas || !gameUI.midDrawRoundRect || !gameUI.paintShapeReference) return;
    jclass paintCls = env->GetObjectClass(gameUI.paintShapeReference);
    jmethodID setColor = env->GetMethodID(paintCls, "setColor", "(I)V");

    // Shadow Mask Pass Layer (Shifted down slightly with soft opacity)
    env->CallVoidMethod(gameUI.paintShapeReference, setColor, gameUI.isCurrentlyDark ? 0x44000000 : 0x22000000);
    env->CallVoidMethod(canvas, gameUI.midDrawRoundRect, left + 4.0f, top + 8.0f, right + 4.0f, bottom + 8.0f, rx, ry, gameUI.paintShapeReference);
}

void drawRoundRectNative(JNIEnv* env, jobject canvas, float left, float top, float right, float bottom, float rx, float ry, int colorHex) {
    if (!canvas || !gameUI.midDrawRoundRect || !gameUI.paintShapeReference) return;
    jclass paintCls = env->GetObjectClass(gameUI.paintShapeReference);
    jmethodID setColor = env->GetMethodID(paintCls, "setColor", "(I)V");
    env->CallVoidMethod(gameUI.paintShapeReference, setColor, colorHex);
    env->CallVoidMethod(canvas, gameUI.midDrawRoundRect, left, top, right, bottom, rx, ry, gameUI.paintShapeReference);
}

int getNextUnlockableLevel() {
    for (int i = 0; i < 50; i++) {
        if (!gameUI.levelsUnlocked[i]) return i;
    }
    return -1;
}

// Utility configuration setting bold status natively
void setPaintFontWeight(JNIEnv* env, jobject paintRef, bool isBold) {
    if (!paintRef) return;
    jclass paintCls = env->GetObjectClass(paintRef);
    jmethodID setFakeBold = env->GetMethodID(paintCls, "setFakeBoldText", "(Z)V");
    if (setFakeBold) {
        env->CallVoidMethod(paintRef, setFakeBold, isBold ? JNI_TRUE : JNI_FALSE);
    }
}

extern "C" {

JNIEXPORT void JNICALL
Java_com_night_backgroundchange_MainActivity_initNativeEngine(JNIEnv* env, jobject obj, jboolean systemDark) {
    gameUI.engineInitialized = false;

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
        gameUI.midDrawLine = env->GetMethodID(gameUI.canvasClass, "drawLine", "(FFFFLandroid/graphics/Paint;)V");
    }

    if (gameUI.bitmapClass) {
        gameUI.midGetWidth = env->GetMethodID(gameUI.bitmapClass, "getWidth", "()I");
        gameUI.midGetHeight = env->GetMethodID(gameUI.bitmapClass, "getHeight", "()I");
    }

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

    // Invert Design Core Tints
    int baseBgColor = gameUI.isCurrentlyDark ? 0xFF121212 : 0xFFFFFFFF;
    int baseTxtColor = gameUI.isCurrentlyDark ? 0xFFFFFFFF : 0xFF000000;
    
    // Configured Active Selection/Unselected States for theme modifications
    int activeSelectionColor = gameUI.isCurrentlyDark ? 0xFFFFFFFF : 0xFF000000;
    int unselectedGrayColor = 0xFF7A7A7A; 

    env->CallVoidMethod(canvas, gameUI.midDrawColor, baseBgColor);
    gameUI.UIButtons.clear();

    if (gameUI.paintTextReference) {
        jclass paintCls = env->GetObjectClass(gameUI.paintTextReference);
        jmethodID setColor = env->GetMethodID(paintCls, "setColor", "(I)V");
        jmethodID setTextSize = env->GetMethodID(paintCls, "setTextSize", "(F)V");
        env->CallVoidMethod(gameUI.paintTextReference, setColor, baseTxtColor);
        env->CallVoidMethod(gameUI.paintTextReference, setTextSize, 45.0f);
        setPaintFontWeight(env, gameUI.paintTextReference, false);
    }

    jobject tintActive = getTintPaint(env, obj, activeSelectionColor);
    jobject tintGray = getTintPaint(env, obj, unselectedGrayColor);
    jobject tintYellow = getTintPaint(env, obj, 0xFFFFCC00);

    // --- SCREEN STATE 1: LOADING IMMERSION ---
    if (gameUI.currentState == STATE_LOADING) {
        gameUI.loadingProgress += 0.02f;
        if (gameUI.loadingProgress >= 1.0f) gameUI.currentState = STATE_HOME;

        jstring loadStr = env->NewStringUTF("LOADING LAYOUT...");
        env->CallVoidMethod(canvas, gameUI.midDrawText, loadStr, (jfloat)(gameUI.screenWidth * 0.25f), (jfloat)(gameUI.screenHeight * 0.5f), gameUI.paintTextReference);
        env->DeleteLocalRef(loadStr);
        return;
    }

    // --- RENDER DYNAMIC NAV FOOTER BAR ---
    float footerHeight = gameUI.screenHeight * 0.13f;
    float footerY = gameUI.screenHeight - footerHeight;

    if (gameUI.currentState == STATE_HOME || gameUI.currentState == STATE_SETTINGS || gameUI.currentState == STATE_LEVELS) {
        drawRoundRectNative(env, canvas, 0, footerY, gameUI.screenWidth, gameUI.screenHeight, 0, 0, gameUI.isCurrentlyDark ? 0xFF1E1E1E : 0xFFF8F9FA);

        // Increased size matrix mapping (+15% larger button allocations)
        float navIconSize = gameUI.screenWidth * 0.095f; 
        float paddingEdge = 60.0f; // Exact crisp alignment offset distance from left and right margins
        float innerSpaceY = footerY + (footerHeight / 2.0f) - (navIconSize / 2.0f);

        // Positioning Left Border: Home
        jobject homePaint = (gameUI.currentState == STATE_HOME) ? tintActive : tintGray;
        if (gameUI.assetBitmaps[ASSET_HOME]) {
            renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_HOME], paddingEdge, innerSpaceY, navIconSize, homePaint);
            gameUI.UIButtons.push_back({paddingEdge, innerSpaceY, navIconSize, navIconSize, 9001, 0});
        }

        // Positioning Centered: Levels
        jobject lvlPaint = (gameUI.currentState == STATE_LEVELS) ? tintActive : tintGray;
        float centerLvlX = (gameUI.screenWidth / 2.0f) - (navIconSize / 2.0f);
        if (gameUI.assetBitmaps[ASSET_LEVEL]) {
            renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_LEVEL], centerLvlX, innerSpaceY, navIconSize, lvlPaint);
            gameUI.UIButtons.push_back({centerLvlX, innerSpaceY, navIconSize, navIconSize, 9002, 0});
        }

        // Positioning Right Border: Settings
        jobject setPaint = (gameUI.currentState == STATE_SETTINGS) ? tintActive : tintGray;
        float rightSetX = gameUI.screenWidth - navIconSize - paddingEdge;
        if (gameUI.assetBitmaps[ASSET_SETTINGS]) {
            renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_SETTINGS], rightSetX, innerSpaceY, navIconSize, setPaint);
            gameUI.UIButtons.push_back({rightSetX, innerSpaceY, navIconSize, navIconSize, 9003, 0});
        }
    }

    // --- STATE 2: HOME SCREEN VIEW ---
    if (gameUI.currentState == STATE_HOME) {
        // "RUN ARROW" Title placed right at the top header area
        setPaintFontWeight(env, gameUI.paintTextReference, true);
        if (gameUI.paintTextReference) {
            jclass paintCls = env->GetObjectClass(gameUI.paintTextReference);
            jmethodID setTextSize = env->GetMethodID(paintCls, "setTextSize", "(F)V");
            env->CallVoidMethod(gameUI.paintTextReference, setTextSize, 95.0f); // Render larger scale
        }
        jstring titleStr = env->NewStringUTF("RUN ARROW");
        env->CallVoidMethod(canvas, gameUI.midDrawText, titleStr, (jfloat)(gameUI.screenWidth * 0.23f), 150.0f, gameUI.paintTextReference);
        env->DeleteLocalRef(titleStr);

        // Restore standard specs
        setPaintFontWeight(env, gameUI.paintTextReference, false);
        if (gameUI.paintTextReference) {
            jclass paintCls = env->GetObjectClass(gameUI.paintTextReference);
            jmethodID setTextSize = env->GetMethodID(paintCls, "setTextSize", "(F)V");
            env->CallVoidMethod(gameUI.paintTextReference, setTextSize, 45.0f);
        }

        // Large Play Button Center Screen
        float playW = gameUI.screenWidth * 0.35f;
        float playX = (gameUI.screenWidth / 2.0f) - (playW / 2.0f);
        float playY = (gameUI.screenHeight * 0.32f);
        if (gameUI.assetBitmaps[ASSET_PLAY]) {
            renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_PLAY], playX, playY, playW, tintActive);
            gameUI.UIButtons.push_back({playX, playY, playW, playW, 2001, 0});
        }

        // Touchable Blue Box Button Container for Played Level Indicator
        float playBoxW = gameUI.screenWidth * 0.65f;
        float playBoxH = 110.0f;
        float playBoxX = (gameUI.screenWidth - playBoxW) / 2.0f;
        float playBoxY = gameUI.screenHeight * 0.62f;

        // Draw shadow map + blue round box layout frame mapping
        drawRealShadowRoundRect(env, canvas, playBoxX, playBoxY, playBoxX + playBoxW, playBoxY + playBoxH, 24, 24);
        drawRoundRectNative(env, canvas, playBoxX, playBoxY, playBoxX + playBoxW, playBoxY + playBoxH, 24, 24, 0xFF007AFF);

        // Render bold text into touchable container
        setPaintFontWeight(env, gameUI.paintTextReference, true);
        if (gameUI.paintTextReference) {
            jclass paintCls = env->GetObjectClass(gameUI.paintTextReference);
            jmethodID setColor = env->GetMethodID(paintCls, "setColor", "(I)V");
            env->CallVoidMethod(gameUI.paintTextReference, setColor, 0xFFFFFFFF); // White text inside blue box
        }

        std::string labelText = "PLAYING LEVEL " + std::to_string(gameUI.currentPlayingLevel);
        jstring jLbl = env->NewStringUTF(labelText.c_str());
        env->CallVoidMethod(canvas, gameUI.midDrawText, jLbl, playBoxX + 55.0f, playBoxY + 70.0f, gameUI.paintTextReference);
        env->DeleteLocalRef(jLbl);

        // Reset system context settings
        if (gameUI.paintTextReference) {
            jclass paintCls = env->GetObjectClass(gameUI.paintTextReference);
            jmethodID setColor = env->GetMethodID(paintCls, "setColor", "(I)V");
            env->CallVoidMethod(gameUI.paintTextReference, setColor, baseTxtColor);
        }
        setPaintFontWeight(env, gameUI.paintTextReference, false);

        // Bind touch code interface tracker mapping
        gameUI.UIButtons.push_back({playBoxX, playBoxY, playBoxW, playBoxH, 2005, 0});
    }

    // --- STATE 3: LEVEL CONFIGURATION SELECTION matrix (50 Levels) ---
    if (gameUI.currentState == STATE_LEVELS) {
        setPaintFontWeight(env, gameUI.paintTextReference, true);
        jstring levelHeader = env->NewStringUTF("SELECT LEVEL");
        env->CallVoidMethod(canvas, gameUI.midDrawText, levelHeader, (jfloat)(gameUI.screenWidth * 0.32f), 90.0f, gameUI.paintTextReference);
        env->DeleteLocalRef(levelHeader);
        setPaintFontWeight(env, gameUI.paintTextReference, false);

        // Render Scrollable/Grid Matrix layout matching requested parameters
        float boxSize = gameUI.screenWidth * 0.15f;
        float spaceGrid = gameUI.screenWidth * 0.035f;
        float offsetGridX = (gameUI.screenWidth - (5 * boxSize + 4 * spaceGrid)) / 2.0f;
        float offsetGridY = 140.0f;

        int nextAdIndex = getNextUnlockableLevel();

        // Matrix Grid: 10 rows, 5 columns = 50 items
        for (int i = 0; i < 50; i++) {
            int row = i / 5;
            int col = i % 5;
            float bx = offsetGridX + col * (boxSize + spaceGrid);
            float by = offsetGridY + row * (boxSize + spaceGrid);

            // Optimization layout checklist boundaries
            if (by + boxSize > footerY) continue; 

            bool isUnlocked = gameUI.levelsUnlocked[i];
            bool isAd = (i == nextAdIndex);

            // Draw shadow base map to look like real standalone buttons
            drawRealShadowRoundRect(env, canvas, bx, by, bx + boxSize, by + boxSize, 20, 20);

            int finalBoxColor = isUnlocked ? 0xFF007AFF : 0xFFD3D3D3; // Blue if unlocked, Light Gray if locked
            drawRoundRectNative(env, canvas, bx, by, bx + boxSize, by + boxSize, 20, 20, finalBoxColor);

            if (!isUnlocked) {
                if (gameUI.assetBitmaps[ASSET_LOCK]) {
                    renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_LOCK], bx + (boxSize * 0.25f), by + (boxSize * 0.25f), boxSize * 0.5f, tintActive);
                }
                if (isAd && gameUI.assetBitmaps[ASSET_WATCH_ADS]) {
                    renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_WATCH_ADS], bx + (boxSize * 0.62f), by + 6.0f, boxSize * 0.32f);
                }
            } else {
                setPaintFontWeight(env, gameUI.paintTextReference, true); // Bold text identifiers
                std::string numLvl = std::to_string(i + 1);
                jstring jNumL = env->NewStringUTF(numLvl.c_str());
                env->CallVoidMethod(canvas, gameUI.midDrawText, jNumL, bx + (boxSize * 0.32f), by + (boxSize * 0.62f), gameUI.paintTextReference);
                env->DeleteLocalRef(jNumL);
                setPaintFontWeight(env, gameUI.paintTextReference, false);
            }

            gameUI.UIButtons.push_back({bx, by, boxSize, boxSize, 3000 + i, i});
        }

        // Bottom Grid Trigger to Complete all levels layout structure safely
        float completeBtnY = offsetGridY + 10 * (boxSize + spaceGrid) + 20.0f;
        if (completeBtnY + 80.0f < footerY) {
            float cW = gameUI.screenWidth * 0.7f;
            float cX = (gameUI.screenWidth - cW) / 2.0f;
            drawRealShadowRoundRect(env, canvas, cX, completeBtnY, cX + cW, completeBtnY + 80.0f, 16, 16);
            drawRoundRectNative(env, canvas, cX, completeBtnY, cX + cW, completeBtnY + 80.0f, 16, 16, 0xFF34C759); // Green box

            jstring completeTxt = env->NewStringUTF("COMPLETE ALL LEVELS");
            env->CallVoidMethod(canvas, gameUI.midDrawText, completeTxt, cX + 60.0f, completeBtnY + 55.0f, gameUI.paintTextReference);
            env->DeleteLocalRef(completeTxt);

            gameUI.UIButtons.push_back({cX, completeBtnY, cW, 80.0f, 3500, 0});
        }
    }

    // --- STATE 4: SETTINGS PANEL DESIGN LAYOUT ---
    if (gameUI.currentState == STATE_SETTINGS) {
        float rowItemY = gameUI.screenHeight * 0.15f;
        float rowItemH = 135.0f;
        float rowItemW = gameUI.screenWidth * 0.9f;
        float rowItemX = (gameUI.screenWidth - rowItemW) / 2.0f;

        jclass paintCls = env->GetObjectClass(gameUI.paintShapeReference);
        jmethodID setColor = env->GetMethodID(paintCls, "setColor", "(I)V");

        // List Row Item 1: Theme Manager Selection Options Trigger Panel
        jstring tTxt = env->NewStringUTF("THEME SELECTION");
        env->CallVoidMethod(canvas, gameUI.midDrawText, tTxt, rowItemX + 10, rowItemY + 80, gameUI.paintTextReference);
        env->DeleteLocalRef(tTxt);
        gameUI.UIButtons.push_back({rowItemX, rowItemY, rowItemW, rowItemH, 6501, 0});

        // Line Separator
        env->CallVoidMethod(gameUI.paintShapeReference, setColor, gameUI.isCurrentlyDark ? 0xFF3A3A3C : 0xFFE5E5EA);
        env->CallVoidMethod(canvas, gameUI.midDrawLine, rowItemX, rowItemY + rowItemH, rowItemX + rowItemW, rowItemY + rowItemH, gameUI.paintShapeReference);

        // List Row Item 2: Privacy Policy Link Box
        rowItemY += rowItemH;
        jstring pTxt = env->NewStringUTF("PRIVACY POLICY");
        env->CallVoidMethod(canvas, gameUI.midDrawText, pTxt, rowItemX + 10, rowItemY + 80, gameUI.paintTextReference);
        env->DeleteLocalRef(pTxt);
        gameUI.UIButtons.push_back({rowItemX, rowItemY, rowItemW, rowItemH, 6502, 0});

        // Line Separator
        env->CallVoidMethod(canvas, gameUI.midDrawLine, rowItemX, rowItemY + rowItemH, rowItemX + rowItemW, rowItemY + rowItemH, gameUI.paintShapeReference);

        // List Row Item 3: Share Our App
        rowItemY += rowItemH;
        jstring sTxt = env->NewStringUTF("SHARE OUR APP");
        env->CallVoidMethod(canvas, gameUI.midDrawText, sTxt, rowItemX + 10, rowItemY + 80, gameUI.paintTextReference);
        env->DeleteLocalRef(sTxt);
        if (gameUI.assetBitmaps[ASSET_SHARE]) {
            renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_SHARE], rowItemX + rowItemW - 90, rowItemY + 35, 65.0f, tintActive);
        }
        gameUI.UIButtons.push_back({rowItemX, rowItemY, rowItemW, rowItemH, 6503, 0});

        // Line Separator
        env->CallVoidMethod(canvas, gameUI.midDrawLine, rowItemX, rowItemY + rowItemH, rowItemX + rowItemW, rowItemY + rowItemH, gameUI.paintShapeReference);

        // List Row Item 4: Rate Our App
        rowItemY += rowItemH;
        jstring rTxt = env->NewStringUTF("RATE OUR APP");
        env->CallVoidMethod(canvas, gameUI.midDrawText, rTxt, rowItemX + 10, rowItemY + 80, gameUI.paintTextReference);
        env->DeleteLocalRef(rTxt);
        if (gameUI.assetBitmaps[ASSET_STAR]) {
            // Render Star Icon significantly larger and clearer
            renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_STAR], rowItemX + rowItemW - 95, rowItemY + 30, 75.0f, tintActive);
        }
        gameUI.UIButtons.push_back({rowItemX, rowItemY, rowItemW, rowItemH, 6504, 0});
    }

    // --- STATE 5: ACTIVE SCREEN GAMEPLAY ---
    if (gameUI.currentState == STATE_GAMEPLAY) {
        float headerIconSize = gameUI.screenWidth * 0.11f;
        float paddingEdge = 40.0f;
        float baseIconY = 55.0f;

        // Position Left Border: Pause Option
        if (gameUI.assetBitmaps[ASSET_PAUSED]) {
            renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_PAUSED], paddingEdge, baseIconY, headerIconSize, tintGray);
            gameUI.UIButtons.push_back({paddingEdge, baseIconY, headerIconSize, headerIconSize, 4002, 0});
        }

        // Position Right Border: Hint Option
        if (gameUI.assetBitmaps[ASSET_HINT]) {
            renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_HINT], gameUI.screenWidth - headerIconSize - paddingEdge, baseIconY, headerIconSize, tintYellow);
            gameUI.UIButtons.push_back({gameUI.screenWidth - headerIconSize - paddingEdge, baseIconY, headerIconSize, headerIconSize, 4003, 0});
        }

        std::string infoStr = "LEVEL " + std::to_string(gameUI.currentPlayingLevel);
        jstring jInfoS = env->NewStringUTF(infoStr.c_str());
        env->CallVoidMethod(canvas, gameUI.midDrawText, jInfoS, (jfloat)(gameUI.screenWidth * 0.4f), 100.0f, gameUI.paintTextReference);
        env->DeleteLocalRef(jInfoS);

        // Simulation handle target
        jstring completeSimTxt = env->NewStringUTF("[ Clear Current Level Link ]");
        env->CallVoidMethod(canvas, gameUI.midDrawText, completeSimTxt, (jfloat)(gameUI.screenWidth * 0.15f), (jfloat)(gameUI.screenHeight * 0.5f), gameUI.paintTextReference);
        env->DeleteLocalRef(completeSimTxt);
        gameUI.UIButtons.push_back({(float)(gameUI.screenWidth * 0.15f), (float)(gameUI.screenHeight * 0.45f), (float)(gameUI.screenWidth * 0.7f), 80.0f, 4001, 0});
    }

    // ========================================================
    // --- DIALOG MODALS SCENARIOS scrim handling (40% BLUR scrim overlay) ---
    // ========================================================
    bool anyPopupVisible = (gameUI.isHintPopupActive || gameUI.isThemePopupActive || gameUI.isRatingPopupActive || gameUI.isPausePopupActive);

    if (anyPopupVisible) {
        // Draw 40% opaque blur mask layer
        drawRoundRectNative(env, canvas, 0, 0, gameUI.screenWidth, gameUI.screenHeight, 0, 0, 0x66000000);

        float dW = gameUI.screenWidth * 0.78f;
        float dH = gameUI.screenHeight * 0.38f;
        float dX = (gameUI.screenWidth - dW) / 2.0f;
        float dY = (gameUI.screenHeight - dH) / 2.0f;

        // Custom Compact specifications for layout popup logic handles
        if (gameUI.isHintPopupActive || gameUI.isRatingPopupActive) {
            dW = gameUI.screenWidth * 0.72f; 
            dH = gameUI.screenHeight * 0.32f;
            dX = (gameUI.screenWidth - dW) / 2.0f;
            dY = (gameUI.screenHeight - dH) / 2.0f;
        }

        // Box base container layer
        drawRoundRectNative(env, canvas, dX, dY, dX + dW, dY + dH, 32, 32, gameUI.isCurrentlyDark ? 0xFF1E1E1E : 0xFFFFFFFF);

        // Rendering Large explicit Close option PNG button bound mapping
        float dialogCloseW = 65.0f; 
        float dialogCloseX = dX + dW - dialogCloseW - 35.0f;
        float dialogCloseY = dY + 35.0f;

        if (gameUI.assetBitmaps[ASSET_CLOSE] && !gameUI.isPausePopupActive) {
            renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_CLOSE], dialogCloseX, dialogCloseY, dialogCloseW, tintActive);
            gameUI.UIButtons.push_back({dialogCloseX - 10, dialogCloseY - 10, dialogCloseW + 20, dialogCloseW + 20, 9999, 0});
        }

        // --- SCENARIO A: COMPACT ADAPTIVE HINT MODAL ---
        if (gameUI.isHintPopupActive) {
            setPaintFontWeight(env, gameUI.paintTextReference, true);
            jstring hMsg = env->NewStringUTF("Need hint?");
            env->CallVoidMethod(canvas, gameUI.midDrawText, hMsg, dX + (dW * 0.32f), dY + 110.0f, gameUI.paintTextReference);
            env->DeleteLocalRef(hMsg);
            setPaintFontWeight(env, gameUI.paintTextReference, false);

            float bW = dW * 0.8f;
            float bH = 90.0f;
            float bX = dX + (dW - bW) / 2.0f;
            float bY = dY + dH - bH - 45.0f;

            drawRoundRectNative(env, canvas, bX, bY, bX + bW, bY + bH, 16, 16, 0xFF007AFF);

            if (gameUI.assetBitmaps[ASSET_WATCH_ADS]) {
                renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_WATCH_ADS], bX + 40.0f, bY + 20.0f, 50.0f);
            }

            if (gameUI.paintTextReference) {
                jclass paintCls = env->GetObjectClass(gameUI.paintTextReference);
                jmethodID setColor = env->GetMethodID(paintCls, "setColor", "(I)V");
                env->CallVoidMethod(gameUI.paintTextReference, setColor, 0xFFFFFFFF);
            }
            jstring btnT = env->NewStringUTF("Watch Ads");
            env->CallVoidMethod(canvas, gameUI.midDrawText, btnT, bX + 115.0f, bY + 62.0f, gameUI.paintTextReference);
            env->DeleteLocalRef(btnT);

            if (gameUI.paintTextReference) {
                jclass paintCls = env->GetObjectClass(gameUI.paintTextReference);
                jmethodID setColor = env->GetMethodID(paintCls, "setColor", "(I)V");
                env->CallVoidMethod(gameUI.paintTextReference, setColor, baseTxtColor);
            }
            gameUI.UIButtons.push_back({bX, bY, bW, bH, 8801, 0});
        }

        // --- SCENARIO B: THREE-THEME ENGINE SELECTION DIALOG ---
        if (gameUI.isThemePopupActive) {
            jstring popTitle = env->NewStringUTF("Choose Theme Mode");
            env->CallVoidMethod(canvas, gameUI.midDrawText, popTitle, dX + 45.0f, dY + 90.0f, gameUI.paintTextReference);
            env->DeleteLocalRef(popTitle);

            float itemSlotY = dY + 140.0f;
            std::string modes[] = {"Normal Light Theme", "Pure Dark Black Theme", "System Default Auto"};

            for (int k = 0; k < 3; k++) {
                int rectColor = (gameUI.activeTheme == k) ? 0xFF007AFF : (gameUI.isCurrentlyDark ? 0xFF2C2C2E : 0xFFE5E5EA);
                drawRoundRectNative(env, canvas, dX + 40.0f, itemSlotY, dX + dW - 40.0f, itemSlotY + 75.0f, 12, 12, rectColor);

                if (gameUI.paintTextReference) {
                    jclass paintCls = env->GetObjectClass(gameUI.paintTextReference);
                    jmethodID setColor = env->GetMethodID(paintCls, "setColor", "(I)V");
                    env->CallVoidMethod(gameUI.paintTextReference, setColor, (gameUI.activeTheme == k) ? 0xFFFFFFFF : baseTxtColor);
                }

                jstring mTxt = env->NewStringUTF(modes[k].c_str());
                env->CallVoidMethod(canvas, gameUI.midDrawText, mTxt, dX + 65.0f, itemSlotY + 52.0f, gameUI.paintTextReference);
                env->DeleteLocalRef(mTxt);

                gameUI.UIButtons.push_back({dX + 40.0f, itemSlotY, dW - 80.0f, 75.0f, 7700 + k, 0});
                itemSlotY += 95.0f;
            }

            if (gameUI.paintTextReference) {
                jclass paintCls = env->GetObjectClass(gameUI.paintTextReference);
                jmethodID setColor = env->GetMethodID(paintCls, "setColor", "(I)V");
                env->CallVoidMethod(gameUI.paintTextReference, setColor, baseTxtColor);
            }
        }

        // --- SCENARIO C: STRUCTURAL FIVE STAR RATING POPUP LINK MAPPING ---
        if (gameUI.isRatingPopupActive) {
            jstring rHead = env->NewStringUTF("Please rate our app");
            env->CallVoidMethod(canvas, gameUI.midDrawText, rHead, dX + 50.0f, dY + 100.0f, gameUI.paintTextReference);
            env->DeleteLocalRef(rHead);

            // Five Stars dynamic rendering tracking loop
            float starBlockW = 60.0f;
            float starSpacerX = 15.0f;
            float starStartX = dX + (dW - (5 * starBlockW + 4 * starSpacerX)) / 2.0f;
            float starY = dY + 140.0f;

            for (int s = 0; s < 5; s++) {
                jobject starTint = (s < gameUI.selectedRatingStars) ? tintYellow : tintGray;
                if (gameUI.assetBitmaps[ASSET_STAR]) {
                    renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_STAR], starStartX + s * (starBlockW + starSpacerX), starY, starBlockW, starTint);
                }
                gameUI.UIButtons.push_back({starStartX + s * (starBlockW + starSpacerX), starY, starBlockW, starBlockW, 7800 + s, 0});
            }

            // Submit Button placement
            float subW = dW * 0.65f;
            float subH = 80.0f;
            float subX = dX + (dW - subW) / 2.0f;
            float subY = dY + dH - subH - 35.0f;

            drawRoundRectNative(env, canvas, subX, subY, subX + subW, subY + subH, 16, 16, 0xFF34C759);

            if (gameUI.paintTextReference) {
                jclass paintCls = env->GetObjectClass(gameUI.paintTextReference);
                jmethodID setColor = env->GetMethodID(paintCls, "setColor", "(I)V");
                env->CallVoidMethod(gameUI.paintTextReference, setColor, 0xFFFFFFFF);
            }
            jstring subTextStr = env->NewStringUTF("Submit");
            env->CallVoidMethod(canvas, gameUI.midDrawText, subTextStr, subX + (subW * 0.35f), subY + 55.0f, gameUI.paintTextReference);
            env->DeleteLocalRef(subTextStr);

            if (gameUI.paintTextReference) {
                jclass paintCls = env->GetObjectClass(gameUI.paintTextReference);
                jmethodID setColor = env->GetMethodID(paintCls, "setColor", "(I)V");
                env->CallVoidMethod(gameUI.paintTextReference, setColor, baseTxtColor);
            }
            gameUI.UIButtons.push_back({subX, subY, subW, subH, 7899, 0});
        }

        // --- SCENARIO D: MODAL PAUSE ENGINE MENU LAYER OVERLAY (No list indexing numbers) ---
        if (gameUI.isPausePopupActive) {
            float listY = dY + 50.0f;
            float elementH = 95.0f;
            float elementW = dW * 0.85f;
            float elementX = dX + (dW - elementW) / 2.0f;

            // Row 1: Resume Play Icon button mapping
            if (gameUI.assetBitmaps[ASSET_PLAY]) {
                renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_PLAY], elementX + 30, listY + 15, 60.0f, tintActive);
            }
            jstring rsm = env->NewStringUTF("PLAY GAME");
            env->CallVoidMethod(canvas, gameUI.midDrawText, rsm, elementX + 130.0f, listY + 60.0f, gameUI.paintTextReference);
            env->DeleteLocalRef(rsm);
            gameUI.UIButtons.push_back({elementX, listY, elementW, elementH, 5501, 0});

            // Row 2: Retry Icon button mapping
            listY += elementH;
            if (gameUI.assetBitmaps[ASSET_RETRY]) {
                renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_RETRY], elementX + 30, listY + 15, 60.0f, tintActive);
            }
            jstring rtr = env->NewStringUTF("RETRY LEVEL");
            env->CallVoidMethod(canvas, gameUI.midDrawText, rtr, elementX + 130.0f, listY + 60.0f, gameUI.paintTextReference);
            env->DeleteLocalRef(rtr);
            gameUI.UIButtons.push_back({elementX, listY, elementW, elementH, 5502, 0});

            // Row 3: Adaptive Sound On / Off asset toggle mapping
            listY += elementH;
            int soundAssetIndex = gameUI.audioEnabled ? ASSET_SOUND_ON : ASSET_SOUND_OFF;
            if (gameUI.assetBitmaps[soundAssetIndex]) {
                renderBmp(env, canvas, gameUI.assetBitmaps[soundAssetIndex], elementX + 30, listY + 15, 60.0f, tintActive);
            }
            std::string audioLabelStr = gameUI.audioEnabled ? "AUDIO ON" : "AUDIO OFF";
            jstring jAudioL = env->NewStringUTF(audioLabelStr.c_str());
            env->CallVoidMethod(canvas, gameUI.midDrawText, jAudioL, elementX + 130.0f, listY + 60.0f, gameUI.paintTextReference);
            env->DeleteLocalRef(jAudioL);
            gameUI.UIButtons.push_back({elementX, listY, elementW, elementH, 5503, 0});

            // Row 4: Home Icon redirection mapping
            listY += elementH;
            if (gameUI.assetBitmaps[ASSET_HOME]) {
                renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_HOME], elementX + 30, listY + 15, 60.0f, tintActive);
            }
            jstring hme = env->NewStringUTF("QUIT TO MENU");
            env->CallVoidMethod(canvas, gameUI.midDrawText, hme, elementX + 130.0f, listY + 60.0f, gameUI.paintTextReference);
            env->DeleteLocalRef(hme);
            gameUI.UIButtons.push_back({elementX, listY, elementW, elementH, 5504, 0});
        }
    }
}

JNIEXPORT void JNICALL
Java_com_night_backgroundchange_MainActivity_nativeOnTouch(JNIEnv* env, jobject obj, jfloat x, jfloat y) {
    if (!gameUI.engineInitialized) return;

    // Filter touch matrices through blocking dialog layers if active
    bool blocksBackground = (gameUI.isHintPopupActive || gameUI.isThemePopupActive || gameUI.isRatingPopupActive || gameUI.isPausePopupActive);

    for (const auto& btn : gameUI.UIButtons) {
        if (x >= btn.x && x <= btn.x + btn.w && y >= btn.y && y <= btn.y + btn.h) {
            
            if (btn.actionCode == 9999) { // Global Close Pop-up mapping code handler
                gameUI.isHintPopupActive = false;
                gameUI.isThemePopupActive = false;
                gameUI.isRatingPopupActive = false;
                gameUI.isPausePopupActive = false;
                return;
            }

            if (gameUI.isThemePopupActive && btn.actionCode >= 7700 && btn.actionCode <= 7702) {
                gameUI.activeTheme = (ThemeMode)(btn.actionCode - 7700);
                gameUI.isThemePopupActive = false;
                
                jclass actCls = env->GetObjectClass(obj);
                jmethodID refreshMid = env->GetMethodID(actCls, "triggerThemeRebuild", "()V");
                if (refreshMid) env->CallVoidMethod(obj, refreshMid);
                return;
            }

            if (gameUI.isRatingPopupActive) {
                if (btn.actionCode >= 7800 && btn.actionCode <= 7804) {
                    gameUI.selectedRatingStars = (btn.actionCode - 7800) + 1;
                } else if (btn.actionCode == 7899) { // Submit rating
                    gameUI.isRatingPopupActive = false;
                }
                return;
            }

            if (gameUI.isHintPopupActive && btn.actionCode == 8801) {
                gameUI.isHintPopupActive = false;
                return;
            }

            if (gameUI.isPausePopupActive) {
                if (btn.actionCode == 5501) gameUI.isPausePopupActive = false; // Play/Resume
                else if (btn.actionCode == 5502) gameUI.isPausePopupActive = false; // Retry
                else if (btn.actionCode == 5503) gameUI.audioEnabled = !gameUI.audioEnabled; // Sound Toggle
                else if (btn.actionCode == 5504) { // Quit to Home Menu
                    gameUI.isPausePopupActive = false;
                    gameUI.currentState = STATE_HOME;
                }
                return;
            }

            if (blocksBackground) return; // Prevent clicking background buttons when popups are up

            // --- Grid Matrix Index touch bindings tracker ---
            if (btn.actionCode >= 3000 && btn.actionCode <= 3049) {
                int index = btn.levelValue;
                int nextIndex = getNextUnlockableLevel();
                if (gameUI.levelsUnlocked[index] || index == nextIndex) {
                    gameUI.levelsUnlocked[index] = true; 
                    gameUI.currentPlayingLevel = index + 1;
                    gameUI.currentState = STATE_GAMEPLAY;
                }
                break;
            }

            switch (btn.actionCode) {
                case 9001: gameUI.currentState = STATE_HOME; break;
                case 9002: gameUI.currentState = STATE_LEVELS; break;
                case 9003: gameUI.currentState = STATE_SETTINGS; break;
                case 2001: gameUI.currentState = STATE_GAMEPLAY; break;
                case 2005: // Touchable Played Level Box Redirects right to the active gameplay state
                    gameUI.currentState = STATE_GAMEPLAY;
                    break;
                case 3500: // Complete All Levels Simulator Button Trigger
                    for(int i=0; i<50; i++) gameUI.levelsUnlocked[i] = true;
                    gameUI.currentState = STATE_LEVELS;
                    break;
                case 4001: // Step tracking win handler
                    if (gameUI.currentPlayingLevel < 50) {
                        gameUI.levelsUnlocked[gameUI.currentPlayingLevel] = true;
                        gameUI.currentPlayingLevel++;
                    }
                    gameUI.currentState = STATE_LEVELS;
                    break;
                case 4002: gameUI.isPausePopupActive = true; break; // Pause Popup Trigger
                case 4003: gameUI.isHintPopupActive = true; break;  // Hint Popup Trigger
                case 6501: gameUI.isThemePopupActive = true; break; // Theme Selector Popup Trigger
                case 6504: // Rate App Popup Trigger
                    gameUI.selectedRatingStars = 0;
                    gameUI.isRatingPopupActive = true; 
                    break;
                case 6502: { // Privacy Policy Intent Dispatcher
                    jclass actCls = env->GetObjectClass(obj);
                    jmethodID midIntent = env->GetMethodID(actCls, "dispatchPrivacyPolicyIntent", "()V");
                    if (midIntent) env->CallVoidMethod(obj, midIntent);
                    break;
                }
                case 6503: { // Share Intent Dispatcher
                    jclass actCls = env->GetObjectClass(obj);
                    jmethodID midShare = env->GetMethodID(actCls, "dispatchShareIntent", "()V");
                    if (midShare) env->CallVoidMethod(obj, midShare);
                    break;
                }
            }
            break;
        }
    }
}

} // extern "C"
