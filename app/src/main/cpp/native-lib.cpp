#include <jni.h>
#include <string>
#include <vector>
#include <cmath>
#include <android/log.h>

#define LOG_TAG "GameEngine"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

enum GameState { STATE_LOADING, STATE_HOME, STATE_SETTINGS, STATE_GAMEPLAY, STATE_LEVELS };
enum ThemeMode { THEME_NORMAL, THEME_BLACK, THEME_SYSTEM };

enum AssetIndex {
    ASSET_ARROW = 0, ASSET_TILE, ASSET_GLOW, ASSET_BACK, ASSET_HOME,
    ASSET_RETRY, ASSET_NEXT, ASSET_PLAY, ASSET_PAUSED, ASSET_SETTINGS,
    ASSET_SOUND_ON, ASSET_SOUND_OFF, ASSET_TICK, ASSET_STAR, ASSET_HINT,
    ASSET_CLOSE, ASSET_LOCK, ASSET_SHARE, ASSET_LEVEL, ASSET_WATCH_ADS, 
    ASSET_REMOVE_ADS, ASSET_COUNT
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

    // Theme Config
    ThemeMode activeTheme = THEME_NORMAL;
    bool isCurrentlyDark = false;

    // 50 Levels Track
    bool levelsUnlocked[50] = { true, false };

    // Scroll Configuration for Level View
    float levelScrollOffset = 0.0f;
    float maxScrollExtent = 0.0f;

    // Popup Modal States
    bool isHintPopupActive = false;
    bool isThemePopupActive = false;
    bool isRatingPopupActive = false;
    bool isPausePopupActive = false;

    int selectedRatingStars = 0;

    jobject assetBitmaps[ASSET_COUNT] = { nullptr };
    std::vector<ClickableButton> UIButtons;

    // Dynamic Reflection Class Pointers
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
    jmethodID midDrawCircle = nullptr;
    jmethodID midGetWidth = nullptr;
    jmethodID midGetHeight = nullptr;

    jobject paintTextReference = nullptr;
    jobject paintShapeReference = nullptr;

    GameMenuStructure() = default;
};

static GameMenuStructure gameUI;

// Utility Rendering Core Wrap
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

void drawRealShadowRoundRect(JNIEnv* env, jobject canvas, float left, float top, float right, float bottom, float rx, float ry) {
    if (!canvas || !gameUI.midDrawRoundRect || !gameUI.paintShapeReference) return;
    jclass paintCls = env->GetObjectClass(gameUI.paintShapeReference);
    jmethodID setColor = env->GetMethodID(paintCls, "setColor", "(I)V");

    env->CallVoidMethod(gameUI.paintShapeReference, setColor, gameUI.isCurrentlyDark ? 0x55000000 : 0x22000000);
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
        gameUI.midDrawCircle = env->GetMethodID(gameUI.canvasClass, "drawCircle", "(FFFLandroid/graphics/Paint;)V");
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

JNIEXPORT void JNICALL
Java_com_night_backgroundchange_MainActivity_nativeApplyScroll(JNIEnv* env, jobject obj, jfloat deltaY) {
    if (gameUI.currentState == STATE_LEVELS) {
        gameUI.levelScrollOffset += deltaY;
        if (gameUI.levelScrollOffset > 0.0f) gameUI.levelScrollOffset = 0.0f;
        if (gameUI.levelScrollOffset < -gameUI.maxScrollExtent) gameUI.levelScrollOffset = -gameUI.maxScrollExtent;
    }
}

jobject getTintPaint(JNIEnv* env, jobject obj, int colorHex) {
    jclass actCls = env->GetObjectClass(obj);
    jmethodID getTintMid = env->GetMethodID(actCls, "getTintedPaint", "(I)Landroid/graphics/Paint;");
    if (getTintMid) return env->CallObjectMethod(obj, getTintMid, colorHex);
    return nullptr;
}

JNIEXPORT void JNICALL
Java_com_night_backgroundchange_MainActivity_nativeRender(JNIEnv* env, jobject obj, jobject canvas) {
    if (!canvas || !gameUI.engineInitialized || !gameUI.midDrawColor) return;

    int baseBgColor = gameUI.isCurrentlyDark ? 0xFF121212 : 0xFFFFFFFF;
    int baseTxtColor = gameUI.isCurrentlyDark ? 0xFFFFFFFF : 0xFF000000;
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
    jobject tintRed = getTintPaint(env, obj, 0xFFFF3B30);

    float footerHeight = gameUI.screenHeight * 0.13f;
    float footerY = gameUI.screenHeight - footerHeight;

    if (gameUI.currentState == STATE_HOME || gameUI.currentState == STATE_SETTINGS || gameUI.currentState == STATE_LEVELS) {
        drawRoundRectNative(env, canvas, 0, footerY, gameUI.screenWidth, gameUI.screenHeight, 0, 0, gameUI.isCurrentlyDark ? 0xFF1E1E1E : 0xFFF8F9FA);

        float navIconSize = gameUI.screenWidth * 0.11f;
        float paddingEdge = 65.0f;
        float innerSpaceY = footerY + (footerHeight / 2.0f) - (navIconSize / 2.0f);

        if (gameUI.assetBitmaps[ASSET_HOME]) {
            jobject homePaint = (gameUI.currentState == STATE_HOME) ? tintActive : tintGray;
            renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_HOME], paddingEdge, innerSpaceY, navIconSize, homePaint);
            gameUI.UIButtons.push_back({paddingEdge, innerSpaceY, navIconSize, navIconSize, 9001, 0});
        }

        if (gameUI.assetBitmaps[ASSET_LEVEL]) {
            jobject lvlPaint = (gameUI.currentState == STATE_LEVELS) ? tintActive : tintGray;
            float centerLvlX = (gameUI.screenWidth / 2.0f) - (navIconSize / 2.0f);
            renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_LEVEL], centerLvlX, innerSpaceY, navIconSize, lvlPaint);
            gameUI.UIButtons.push_back({centerLvlX, innerSpaceY, navIconSize, navIconSize, 9002, 0});
        }

        if (gameUI.assetBitmaps[ASSET_SETTINGS]) {
            jobject setPaint = (gameUI.currentState == STATE_SETTINGS) ? tintActive : tintGray;
            float rightSetX = gameUI.screenWidth - navIconSize - paddingEdge;
            renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_SETTINGS], rightSetX, innerSpaceY, navIconSize, setPaint);
            gameUI.UIButtons.push_back({rightSetX, innerSpaceY, navIconSize, navIconSize, 9003, 0});
        }
    }

    if (gameUI.currentState == STATE_LOADING) {
        gameUI.loadingProgress += 0.02f;
        if (gameUI.loadingProgress >= 1.0f) gameUI.currentState = STATE_HOME;
        return;
    }

    if (gameUI.currentState == STATE_HOME) {
        float textStartX = gameUI.screenWidth * 0.18f;
        float textStartY = 160.0f;

        setPaintFontWeight(env, gameUI.paintTextReference, true);
        if (gameUI.paintTextReference) {
            jclass paintCls = env->GetObjectClass(gameUI.paintTextReference);
            jmethodID setTextSize = env->GetMethodID(paintCls, "setTextSize", "(F)V");
            env->CallVoidMethod(gameUI.paintTextReference, setTextSize, 90.0f);
        }

        jstring titlePart1 = env->NewStringUTF("RUN   RROW");
        env->CallVoidMethod(canvas, gameUI.midDrawText, titlePart1, textStartX, textStartY, gameUI.paintTextReference);
        env->DeleteLocalRef(titlePart1);

        float customArrowSize = 95.0f; 
        float customArrowX = textStartX + 225.0f;
        float customArrowY = textStartY - 82.0f;
        if (gameUI.assetBitmaps[ASSET_ARROW]) {
            renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_ARROW], customArrowX, customArrowY, customArrowSize, tintRed);
        }

        float removeAdsX = gameUI.screenWidth - 130.0f;
        float removeAdsY = textStartY - 75.0f;
        if (gameUI.assetBitmaps[ASSET_REMOVE_ADS]) {
            renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_REMOVE_ADS], removeAdsX, removeAdsY, 90.0f, tintActive);
            gameUI.UIButtons.push_back({removeAdsX, removeAdsY, 90.0f, 90.0f, 2010, 0});
        }

        setPaintFontWeight(env, gameUI.paintTextReference, false);
        if (gameUI.paintTextReference) {
            jclass paintCls = env->GetObjectClass(gameUI.paintTextReference);
            jmethodID setTextSize = env->GetMethodID(paintCls, "setTextSize", "(F)V");
            env->CallVoidMethod(gameUI.paintTextReference, setTextSize, 45.0f);
        }

        float wmSize = gameUI.screenWidth * 0.36f;
        float wmX = (gameUI.screenWidth - wmSize) / 2.0f;
        float wmY = gameUI.screenHeight * 0.28f;
        
        drawRoundRectNative(env, canvas, wmX, wmY, wmX + wmSize, wmY + wmSize, 28, 28, gameUI.isCurrentlyDark ? 0xFF1C1C1E : 0xFFF2F2F7);

        float iconItemSize = wmSize * 0.38f;
        jobject wmTint = getTintPaint(env, obj, gameUI.isCurrentlyDark ? 0xFF2C2C2E : 0xFFD1D1D6);
        
        env->CallIntMethod(canvas, gameUI.midSave);
        env->CallVoidMethod(canvas, gameUI.midTranslate, wmX + (wmSize / 2.0f), wmY + (wmSize / 3.0f));
        env->CallVoidMethod(canvas, gameUI.midScale, 1.0f, -1.0f, 0.0f, 0.0f);
        if (gameUI.assetBitmaps[ASSET_ARROW]) {
            renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_ARROW], -iconItemSize / 2.0f, -iconItemSize / 2.0f, iconItemSize, wmTint);
        }
        env->CallVoidMethod(canvas, gameUI.midRestore);

        env->CallIntMethod(canvas, gameUI.midSave);
        env->CallVoidMethod(canvas, gameUI.midTranslate, wmX + (wmSize / 3.0f), wmY + (wmSize * 0.68f));
        jclass canvasCls = env->GetObjectClass(canvas);
        jmethodID midRotate = env->GetMethodID(canvasCls, "rotate", "(FFF)V");
        if (midRotate) env->CallVoidMethod(canvas, midRotate, -90.0f, 0.0f, 0.0f);
        if (gameUI.assetBitmaps[ASSET_ARROW]) {
            renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_ARROW], -iconItemSize / 2.0f, -iconItemSize / 2.0f, iconItemSize, wmTint);
        }
        env->CallVoidMethod(canvas, gameUI.midRestore);

        env->CallIntMethod(canvas, gameUI.midSave);
        env->CallVoidMethod(canvas, gameUI.midTranslate, wmX + (wmSize * 0.68f), wmY + (wmSize * 0.68f));
        if (midRotate) env->CallVoidMethod(canvas, midRotate, 90.0f, 0.0f, 0.0f);
        if (gameUI.assetBitmaps[ASSET_ARROW]) {
            renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_ARROW], -iconItemSize / 2.0f, -iconItemSize / 2.0f, iconItemSize, wmTint);
        }
        env->CallVoidMethod(canvas, gameUI.midRestore);

        float playW = gameUI.screenWidth * 0.45f;
        float playX = (gameUI.screenWidth / 2.0f) - (playW / 2.0f);
        float playY = gameUI.screenHeight * 0.48f;
        
        if (gameUI.assetBitmaps[ASSET_PLAY]) {
            renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_PLAY], playX, playY, playW, tintActive);
            gameUI.UIButtons.push_back({playX, playY, playW, playW, 2001, 0});
        }
    }

    if (gameUI.currentState == STATE_LEVELS) {
        setPaintFontWeight(env, gameUI.paintTextReference, true);
        jstring levelHeader = env->NewStringUTF("SELECT LEVEL");
        env->CallVoidMethod(canvas, gameUI.midDrawText, levelHeader, (jfloat)(gameUI.screenWidth * 0.32f), 85.0f, gameUI.paintTextReference);
        env->DeleteLocalRef(levelHeader);
        setPaintFontWeight(env, gameUI.paintTextReference, false);

        float boxSize = gameUI.screenWidth * 0.25f;
        float spaceGrid = gameUI.screenWidth * 0.045f;
        float offsetGridX = (gameUI.screenWidth - (3 * boxSize + 2 * spaceGrid)) / 2.0f;
        float offsetGridY = 140.0f;

        int nextAdIndex = getNextUnlockableLevel();
        int totalRows = (int)std::ceil(50.0f / 3.0f);
        float totalContentHeight = totalRows * (boxSize + spaceGrid) + 160.0f;
        gameUI.maxScrollExtent = totalContentHeight - (footerY - offsetGridY);
        if (gameUI.maxScrollExtent < 0.0f) gameUI.maxScrollExtent = 0.0f;

        env->CallIntMethod(canvas, gameUI.midSave);
        
        for (int i = 0; i < 50; i++) {
            int row = i / 3;
            int col = i % 3;
            float bx = offsetGridX + col * (boxSize + spaceGrid);
            float by = offsetGridY + row * (boxSize + spaceGrid) + gameUI.levelScrollOffset;

            if (by + boxSize < offsetGridY || by > footerY) continue;

            drawRealShadowRoundRect(env, canvas, bx, by, bx + boxSize, by + boxSize, 24, 24);
            int finalBoxColor = gameUI.levelsUnlocked[i] ? 0xFF007AFF : 0xFFD3D3D3;
            drawRoundRectNative(env, canvas, bx, by, bx + boxSize, by + boxSize, 24, 24, finalBoxColor);

            if (!gameUI.levelsUnlocked[i]) {
                if (gameUI.assetBitmaps[ASSET_LOCK]) {
                    renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_LOCK], bx + (boxSize * 0.28f), by + (boxSize * 0.28f), boxSize * 0.44f, tintActive);
                }
                if (i == nextAdIndex && gameUI.assetBitmaps[ASSET_WATCH_ADS]) {
                    renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_WATCH_ADS], bx + (boxSize * 0.65f), by + 8.0f, boxSize * 0.28f);
                }
            } else {
                setPaintFontWeight(env, gameUI.paintTextReference, true);
                std::string numLvl = std::to_string(i + 1);
                jstring jNumL = env->NewStringUTF(numLvl.c_str());
                env->CallVoidMethod(canvas, gameUI.midDrawText, jNumL, bx + (boxSize * 0.36f), by + (boxSize * 0.60f), gameUI.paintTextReference);
                env->DeleteLocalRef(jNumL);
                setPaintFontWeight(env, gameUI.paintTextReference, false);
            }

            gameUI.UIButtons.push_back({bx, by, boxSize, boxSize, 3000 + i, i});
        }

        float completeBtnY = offsetGridY + totalRows * (boxSize + spaceGrid) + gameUI.levelScrollOffset + 20.0f;
        if (completeBtnY + 85.0f > offsetGridY && completeBtnY < footerY) {
            float cW = gameUI.screenWidth * 0.75f;
            float cX = (gameUI.screenWidth - cW) / 2.0f;
            drawRealShadowRoundRect(env, canvas, cX, completeBtnY, cX + cW, completeBtnY + 85.0f, 20, 20);
            drawRoundRectNative(env, canvas, cX, completeBtnY, cX + cW, completeBtnY + 85.0f, 20, 20, 0xFF34C759);

            jstring completeTxt = env->NewStringUTF("COMPLETE ALL LEVELS");
            env->CallVoidMethod(canvas, gameUI.midDrawText, completeTxt, cX + 75.0f, completeBtnY + 58.0f, gameUI.paintTextReference);
            env->DeleteLocalRef(completeTxt);

            gameUI.UIButtons.push_back({cX, completeBtnY, cW, 85.0f, 3500, 0});
        }

        env->CallVoidMethod(canvas, gameUI.midRestore);
    }

    if (gameUI.currentState == STATE_GAMEPLAY) {
        float headerIconSize = gameUI.screenWidth * 0.11f;
        float baseIconY = 55.0f;

        if (gameUI.assetBitmaps[ASSET_PAUSED]) {
            renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_PAUSED], 40.0f, baseIconY, headerIconSize, tintGray);
            gameUI.UIButtons.push_back({40.0f, baseIconY, headerIconSize, headerIconSize, 4002, 0});
        }
        if (gameUI.assetBitmaps[ASSET_HINT]) {
            renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_HINT], gameUI.screenWidth - headerIconSize - 40.0f, baseIconY, headerIconSize, tintYellow);
            gameUI.UIButtons.push_back({gameUI.screenWidth - headerIconSize - 40.0f, baseIconY, headerIconSize, headerIconSize, 4003, 0});
        }

        float dotPlayAreaTop = 200.0f;
        float dotPlayAreaBottom = footerY - 40.0f;
        float dotFieldH = dotPlayAreaBottom - dotPlayAreaTop;
        
        int dotRows = 5;
        int dotCols = 10;
        float spacingRow = dotFieldH / (dotRows + 1);
        float spacingCol = gameUI.screenWidth / (dotCols + 1);

        if (gameUI.paintShapeReference) {
            jclass paintCls = env->GetObjectClass(gameUI.paintShapeReference);
            jmethodID setColor = env->GetMethodID(paintCls, "setColor", "(I)V");
            env->CallVoidMethod(gameUI.paintShapeReference, setColor, gameUI.isCurrentlyDark ? 0xFF444446 : 0xFFD1D1D6);

            for (int r = 0; r < dotRows; r++) {
                for (int c = 0; c < dotCols; c++) {
                    float dotX = spacingCol * (c + 1);
                    float dotY = dotPlayAreaTop + spacingRow * (r + 1);
                    env->CallVoidMethod(canvas, gameUI.midDrawCircle, dotX, dotY, 9.0f, gameUI.paintShapeReference);
                }
            }
        }

        jstring simH = env->NewStringUTF("[ Tap To Clear Level ]");
        env->CallVoidMethod(canvas, gameUI.midDrawText, simH, gameUI.screenWidth * 0.28f, gameUI.screenHeight * 0.85f, gameUI.paintTextReference);
        env->DeleteLocalRef(simH);
        gameUI.UIButtons.push_back({gameUI.screenWidth * 0.25f, gameUI.screenHeight * 0.80f, gameUI.screenWidth * 0.5f, 80.0f, 4001, 0});
    }

    if (gameUI.currentState == STATE_SETTINGS) {
        float itemRowY = gameUI.screenHeight * 0.12f;
        float itemRowH = 160.0f; 
        float itemRowW = gameUI.screenWidth * 0.92f;
        float itemRowX = (gameUI.screenWidth - itemRowW) / 2.0f;

        jclass paintCls = env->GetObjectClass(gameUI.paintShapeReference);
        jmethodID setColor = env->GetMethodID(paintCls, "setColor", "(I)V");

        jstring tText = env->NewStringUTF("Theme Selection");
        env->CallVoidMethod(canvas, gameUI.midDrawText, tText, itemRowX + 15.0f, itemRowY + 65.0f, gameUI.paintTextReference);
        env->DeleteLocalRef(tText);
        
        if (gameUI.paintTextReference) {
            jclass pTxtCls = env->GetObjectClass(gameUI.paintTextReference);
            jmethodID setSize = env->GetMethodID(pTxtCls, "setTextSize", "(F)V");
            jmethodID setCol = env->GetMethodID(pTxtCls, "setColor", "(I)V");
            env->CallVoidMethod(gameUI.paintTextReference, setSize, 32.0f);
            env->CallVoidMethod(gameUI.paintTextReference, setCol, unselectedGrayColor);
            jstring tDesc = env->NewStringUTF("Customize interface look and dark accent control panels");
            env->CallVoidMethod(canvas, gameUI.midDrawText, tDesc, itemRowX + 15.0f, itemRowY + 115.0f, gameUI.paintTextReference);
            env->DeleteLocalRef(tDesc);
            env->CallVoidMethod(gameUI.paintTextReference, setSize, 45.0f);
            env->CallVoidMethod(gameUI.paintTextReference, setCol, baseTxtColor);
        }
        gameUI.UIButtons.push_back({itemRowX, itemRowY, itemRowW, itemRowH, 6501, 0});

        env->CallVoidMethod(gameUI.paintShapeReference, setColor, gameUI.isCurrentlyDark ? 0xFF2C2C2E : 0xFFE5E5EA);
        env->CallVoidMethod(canvas, gameUI.midDrawLine, itemRowX, itemRowY + itemRowH, itemRowX + itemRowW, itemRowY + itemRowH, gameUI.paintShapeReference);

        itemRowY += itemRowH;
        jstring pText = env->NewStringUTF("Privacy Policy");
        env->CallVoidMethod(canvas, gameUI.midDrawText, pText, itemRowX + 15.0f, itemRowY + 65.0f, gameUI.paintTextReference);
        env->DeleteLocalRef(pText);

        if (gameUI.paintTextReference) {
            jclass pTxtCls = env->GetObjectClass(gameUI.paintTextReference);
            jmethodID setSize = env->GetMethodID(pTxtCls, "setTextSize", "(F)V");
            jmethodID setCol = env->GetMethodID(pTxtCls, "setColor", "(I)V");
            env->CallVoidMethod(gameUI.paintTextReference, setSize, 32.0f);
            env->CallVoidMethod(gameUI.paintTextReference, setCol, unselectedGrayColor);
            jstring pDesc = env->NewStringUTF("Review our online user data privacy protection guidelines");
            env->CallVoidMethod(canvas, gameUI.midDrawText, pDesc, itemRowX + 15.0f, itemRowY + 115.0f, gameUI.paintTextReference);
            env->DeleteLocalRef(pDesc);
            env->CallVoidMethod(gameUI.paintTextReference, setSize, 45.0f);
            env->CallVoidMethod(gameUI.paintTextReference, setCol, baseTxtColor);
        }
        gameUI.UIButtons.push_back({itemRowX, itemRowY, itemRowW, itemRowH, 6502, 0});

        env->CallVoidMethod(canvas, gameUI.midDrawLine, itemRowX, itemRowY + itemRowH, itemRowX + itemRowW, itemRowY + itemRowH, gameUI.paintShapeReference);

        itemRowY += itemRowH;
        jstring sText = env->NewStringUTF("Share Our App");
        env->CallVoidMethod(canvas, gameUI.midDrawText, sText, itemRowX + 15.0f, itemRowY + 65.0f, gameUI.paintTextReference);
        env->DeleteLocalRef(sText);

        if (gameUI.paintTextReference) {
            jclass pTxtCls = env->GetObjectClass(gameUI.paintTextReference);
            jmethodID setSize = env->GetMethodID(pTxtCls, "setTextSize", "(F)V");
            jmethodID setCol = env->GetMethodID(pTxtCls, "setColor", "(I)V");
            env->CallVoidMethod(gameUI.paintTextReference, setSize, 32.0f);
            env->CallVoidMethod(gameUI.paintTextReference, setCol, unselectedGrayColor);
            jstring sDesc = env->NewStringUTF("Share game link with family and social networks");
            env->CallVoidMethod(canvas, gameUI.midDrawText, sDesc, itemRowX + 15.0f, itemRowY + 115.0f, gameUI.paintTextReference);
            env->DeleteLocalRef(sDesc);
            env->CallVoidMethod(gameUI.paintTextReference, setSize, 45.0f);
            env->CallVoidMethod(gameUI.paintTextReference, setCol, baseTxtColor);
        }
        if (gameUI.assetBitmaps[ASSET_SHARE]) {
            renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_SHARE], itemRowX + itemRowW - 95, itemRowY + 45, 70.0f, tintActive);
        }
        gameUI.UIButtons.push_back({itemRowX, itemRowY, itemRowW, itemRowH, 6503, 0});

        env->CallVoidMethod(canvas, gameUI.midDrawLine, itemRowX, itemRowY + itemRowH, itemRowX + itemRowW, itemRowY + itemRowH, gameUI.paintShapeReference);

        itemRowY += itemRowH;
        jstring rText = env->NewStringUTF("Rate Our App");
        env->CallVoidMethod(canvas, gameUI.midDrawText, rText, itemRowX + 15.0f, itemRowY + 65.0f, gameUI.paintTextReference);
        env->DeleteLocalRef(rText);

        if (gameUI.paintTextReference) {
            jclass pTxtCls = env->GetObjectClass(gameUI.paintTextReference);
            jmethodID setSize = env->GetMethodID(pTxtCls, "setTextSize", "(F)V");
            jmethodID setCol = env->GetMethodID(pTxtCls, "setColor", "(I)V");
            env->CallVoidMethod(gameUI.paintTextReference, setSize, 32.0f);
            env->CallVoidMethod(gameUI.paintTextReference, setCol, unselectedGrayColor);
            jstring rDesc = env->NewStringUTF("Submit active 5 star reviews on storefront networks");
            env->CallVoidMethod(canvas, gameUI.midDrawText, rDesc, itemRowX + 15.0f, itemRowY + 115.0f, gameUI.paintTextReference);
            env->DeleteLocalRef(rDesc);
            env->CallVoidMethod(gameUI.paintTextReference, setSize, 45.0f);
            env->CallVoidMethod(gameUI.paintTextReference, setCol, baseTxtColor);
        }
        if (gameUI.assetBitmaps[ASSET_STAR]) {
            renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_STAR], itemRowX + itemRowW - 95, itemRowY + 45, 70.0f, tintActive);
        }
        gameUI.UIButtons.push_back({itemRowX, itemRowY, itemRowW, itemRowH, 6504, 0});
    }

    bool activeModalBlocks = (gameUI.isHintPopupActive || gameUI.isThemePopupActive || gameUI.isRatingPopupActive || gameUI.isPausePopupActive);

    if (activeModalBlocks) {
        drawRoundRectNative(env, canvas, 0, 0, gameUI.screenWidth, gameUI.screenHeight, 0, 0, 0x66000000);

        float dW = gameUI.screenWidth * 0.76f;
        float dH = gameUI.screenHeight * 0.32f;
        float dX = (gameUI.screenWidth - dW) / 2.0f;
        float dY = (gameUI.screenHeight - dH) / 2.0f;

        if (gameUI.isThemePopupActive || gameUI.isPausePopupActive) {
            dW = gameUI.screenWidth * 0.82f;
            dH = gameUI.screenHeight * 0.44f;
            dX = (gameUI.screenWidth - dW) / 2.0f;
            dY = (gameUI.screenHeight - dH) / 2.0f;
        }

        drawRoundRectNative(env, canvas, dX, dY, dX + dW, dY + dH, 36, 36, gameUI.isCurrentlyDark ? 0xFF1E1E1E : 0xFFFFFFFF);

        float uniformCloseSize = 75.0f; 
        float uniformCloseX = dX + dW - uniformCloseSize - 30.0f;
        float uniformCloseY = dY + 30.0f;

        if (gameUI.assetBitmaps[ASSET_CLOSE] && !gameUI.isPausePopupActive) {
            renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_CLOSE], uniformCloseX, uniformCloseY, uniformCloseSize, tintActive);
            gameUI.UIButtons.push_back({uniformCloseX - 10, uniformCloseY - 10, uniformCloseSize + 20, uniformCloseSize + 20, 9999, 0});
        }

        if (gameUI.isHintPopupActive) {
            setPaintFontWeight(env, gameUI.paintTextReference, true);
            jstring hMsg = env->NewStringUTF("Need hint?");
            env->CallVoidMethod(canvas, gameUI.midDrawText, hMsg, dX + (dW * 0.30f), dY + 105.0f, gameUI.paintTextReference);
            env->DeleteLocalRef(hMsg);
            setPaintFontWeight(env, gameUI.paintTextReference, false);

            float buttonHeight = 105.0f;
            float bW = dW * 0.85f;
            float bX = dX + (dW - bW) / 2.0f;
            float bY = dY + dH - buttonHeight - 40.0f;

            drawRoundRectNative(env, canvas, bX, bY, bX + bW, bY + buttonHeight, 20, 20, 0xFF007AFF);

            float innerIconW = 50.0f;
            float compoundSpacing = 20.0f;
            float totalCompoundWidth = innerIconW + compoundSpacing + 200.0f; 
            float targetStartContentX = bX + (bW - totalCompoundWidth) / 2.0f;

            if (gameUI.assetBitmaps[ASSET_WATCH_ADS]) {
                renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_WATCH_ADS], targetStartContentX, bY + (buttonHeight - innerIconW) / 2.0f, innerIconW);
            }
            if (gameUI.paintTextReference) {
                jclass paintCls = env->GetObjectClass(gameUI.paintTextReference);
                jmethodID setCol = env->GetMethodID(paintCls, "setColor", "(I)V");
                env->CallVoidMethod(gameUI.paintTextReference, setCol, 0xFFFFFFFF);
            }
            jstring adLabel = env->NewStringUTF("Watch Ads");
            env->CallVoidMethod(canvas, gameUI.midDrawText, adLabel, targetStartContentX + innerIconW + compoundSpacing, bY + (buttonHeight / 2.0f) + 16.0f, gameUI.paintTextReference);
            env->DeleteLocalRef(adLabel);
            
            if (gameUI.paintTextReference) {
                jclass paintCls = env->GetObjectClass(gameUI.paintTextReference);
                jmethodID setCol = env->GetMethodID(paintCls, "setColor", "(I)V");
                env->CallVoidMethod(gameUI.paintTextReference, setCol, baseTxtColor);
            }
            gameUI.UIButtons.push_back({bX, bY, bW, buttonHeight, 8801, 0});
        }

        if (gameUI.isThemePopupActive) {
            jstring popTitle = env->NewStringUTF("Choose Theme Mode");
            env->CallVoidMethod(canvas, gameUI.midDrawText, popTitle, dX + 45.0f, dY + 80.0f, gameUI.paintTextReference);
            env->DeleteLocalRef(popTitle);

            float slotY = dY + 130.0f;
            std::string items[] = {"Normal Light Theme", "Pure Dark Black Theme", "System Default Auto"};

            for (int k = 0; k < 3; k++) {
                int bgBoxColor = (gameUI.activeTheme == k) ? 0xFF007AFF : (gameUI.isCurrentlyDark ? 0xFF2C2C2E : 0xFFE5E5EA);
                drawRoundRectNative(env, canvas, dX + 40.0f, slotY, dX + dW - 40.0f, slotY + 75.0f, 16, 16, bgBoxColor);

                if (gameUI.paintTextReference) {
                    jclass paintCls = env->GetObjectClass(gameUI.paintTextReference);
                    jmethodID setCol = env->GetMethodID(paintCls, "setColor", "(I)V");
                    env->CallVoidMethod(gameUI.paintTextReference, setCol, (gameUI.activeTheme == k) ? 0xFFFFFFFF : baseTxtColor);
                }
                jstring mTxt = env->NewStringUTF(items[k].c_str());
                env->CallVoidMethod(canvas, gameUI.midDrawText, mTxt, dX + 65.0f, slotY + 52.0f, gameUI.paintTextReference);
                env->DeleteLocalRef(mTxt);

                gameUI.UIButtons.push_back({dX + 40.0f, slotY, dW - 80.0f, 75.0f, 7700 + k, 0});
                slotY += 92.0f;
            }
            if (gameUI.paintTextReference) {
                jclass paintCls = env->GetObjectClass(gameUI.paintTextReference);
                jmethodID setCol = env->GetMethodID(paintCls, "setColor", "(I)V");
                env->CallVoidMethod(gameUI.paintTextReference, setCol, baseTxtColor);
            }
        }

        if (gameUI.isRatingPopupActive) {
            setPaintFontWeight(env, gameUI.paintTextReference, true);
            jstring rHead = env->NewStringUTF("Please rate our app");
            env->CallVoidMethod(canvas, gameUI.midDrawText, rHead, dX + (dW * 0.16f), dY + 95.0f, gameUI.paintTextReference);
            env->DeleteLocalRef(rHead);
            setPaintFontWeight(env, gameUI.paintTextReference, false);

            float starSize = 70.0f; 
            float starSpacerX = 12.0f;
            float starStartX = dX + (dW - (5 * starSize + 4 * starSpacerX)) / 2.0f;
            float starY = dY + 130.0f;

            for (int s = 0; s < 5; s++) {
                jobject starPaint = (s < gameUI.selectedRatingStars) ? tintYellow : tintGray;
                if (gameUI.assetBitmaps[ASSET_STAR]) {
                    renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_STAR], starStartX + s * (starSize + starSpacerX), starY, starSize, starPaint);
                }
                gameUI.UIButtons.push_back({starStartX + s * (starSize + starSpacerX), starY, starSize, starSize, 7800 + s, 0});
            }

            float subW = dW * 0.70f;
            float subH = 85.0f;
            float subX = dX + (dW - subW) / 2.0f;
            float subY = dY + dH - subH - 30.0f;

            drawRoundRectNative(env, canvas, subX, subY, subX + subW, subY + subH, 16, 16, 0xFF34C759);

            if (gameUI.paintTextReference) {
                jclass paintCls = env->GetObjectClass(gameUI.paintTextReference);
                jmethodID setCol = env->GetMethodID(paintCls, "setColor", "(I)V");
                env->CallVoidMethod(gameUI.paintTextReference, setCol, 0xFFFFFFFF);
            }
            jstring subTxt = env->NewStringUTF("Submit");
            env->CallVoidMethod(canvas, gameUI.midDrawText, subTxt, subX + (subW * 0.36f), subY + 58.0f, gameUI.paintTextReference);
            env->DeleteLocalRef(subTxt);
            if (gameUI.paintTextReference) {
                jclass paintCls = env->GetObjectClass(gameUI.paintTextReference);
                jmethodID setCol = env->GetMethodID(paintCls, "setColor", "(I)V");
                env->CallVoidMethod(gameUI.paintTextReference, setCol, baseTxtColor);
            }
            gameUI.UIButtons.push_back({subX, subY, subW, subH, 7899, 0});
        }

        if (gameUI.isPausePopupActive) {
            float listY = dY + 45.0f;
            float elementH = 95.0f;
            float elementW = dW * 0.88f;
            float elementX = dX + (dW - elementW) / 2.0f;

            if (gameUI.assetBitmaps[ASSET_PLAY]) {
                renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_PLAY], elementX + 35, listY + 15, 65.0f, tintActive);
            }
            jstring rsm = env->NewStringUTF("PLAY GAME");
            env->CallVoidMethod(canvas, gameUI.midDrawText, rsm, elementX + 135.0f, listY + 62.0f, gameUI.paintTextReference);
            env->DeleteLocalRef(rsm);
            gameUI.UIButtons.push_back({elementX, listY, elementW, elementH, 5501, 0});

            listY += elementH;
            if (gameUI.assetBitmaps[ASSET_RETRY]) {
                renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_RETRY], elementX + 35, listY + 15, 65.0f, tintActive);
            }
            jstring rtr = env->NewStringUTF("RETRY LEVEL");
            env->CallVoidMethod(canvas, gameUI.midDrawText, rtr, elementX + 135.0f, listY + 62.0f, gameUI.paintTextReference);
            env->DeleteLocalRef(rtr);
            gameUI.UIButtons.push_back({elementX, listY, elementW, elementH, 5502, 0});

            listY += elementH;
            int sndIndex = gameUI.audioEnabled ? ASSET_SOUND_ON : ASSET_SOUND_OFF;
            if (gameUI.assetBitmaps[sndIndex]) {
                renderBmp(env, canvas, gameUI.assetBitmaps[sndIndex], elementX + 35, listY + 15, 65.0f, tintActive);
            }
            std::string audioStringLabel = gameUI.audioEnabled ? "AUDIO ON" : "AUDIO OFF";
            jstring jAudioL = env->NewStringUTF(audioStringLabel.c_str());
            env->CallVoidMethod(canvas, gameUI.midDrawText, jAudioL, elementX + 135.0f, listY + 62.0f, gameUI.paintTextReference);
            env->DeleteLocalRef(jAudioL);
            gameUI.UIButtons.push_back({elementX, listY, elementW, elementH, 5503, 0});

            listY += elementH;
            if (gameUI.assetBitmaps[ASSET_HOME]) {
                renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_HOME], elementX + 35, listY + 15, 65.0f, tintActive);
            }
            jstring hme = env->NewStringUTF("QUIT TO MENU");
            env->CallVoidMethod(canvas, gameUI.midDrawText, hme, elementX + 135.0f, listY + 62.0f, gameUI.paintTextReference);
            env->DeleteLocalRef(hme);
            gameUI.UIButtons.push_back({elementX, listY, elementW, elementH, 5504, 0});
        }
    }
}

JNIEXPORT void JNICALL
Java_com_night_backgroundchange_MainActivity_nativeOnTouch(JNIEnv* env, jobject obj, jfloat x, jfloat y) {
    if (!gameUI.engineInitialized) return;

    bool blockingPopup = (gameUI.isHintPopupActive || gameUI.isThemePopupActive || gameUI.isRatingPopupActive || gameUI.isPausePopupActive);

    for (const auto& btn : gameUI.UIButtons) {
        if (x >= btn.x && x <= btn.x + btn.w && y >= btn.y && y <= btn.y + btn.h) {
            
            if (btn.actionCode == 9999) { 
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
                } else if (btn.actionCode == 7899) {
                    gameUI.isRatingPopupActive = false;
                }
                return;
            }

            if (gameUI.isHintPopupActive && btn.actionCode == 8801) {
                gameUI.isHintPopupActive = false;
                return;
            }

            if (gameUI.isPausePopupActive) {
                if (btn.actionCode == 5501) gameUI.isPausePopupActive = false;
                else if (btn.actionCode == 5502) gameUI.isPausePopupActive = false;
                else if (btn.actionCode == 5503) gameUI.audioEnabled = !gameUI.audioEnabled;
                else if (btn.actionCode == 5504) {
                    gameUI.isPausePopupActive = false;
                    gameUI.currentState = STATE_HOME;
                }
                return;
            }

            if (blockingPopup) return; 

            if (btn.actionCode >= 3000 && btn.actionCode <= 3049) {
                int lvlIdx = btn.levelValue;
                int nextIdx = getNextUnlockableLevel();
                if (gameUI.levelsUnlocked[lvlIdx] || lvlIdx == nextIdx) {
                    gameUI.levelsUnlocked[lvlIdx] = true;
                    gameUI.currentPlayingLevel = lvlIdx + 1;
                    gameUI.currentState = STATE_GAMEPLAY;
                }
                break;
            }

            switch (btn.actionCode) {
                case 9001: gameUI.currentState = STATE_HOME; break;
                case 9002: gameUI.currentState = STATE_LEVELS; break;
                case 9003: gameUI.currentState = STATE_SETTINGS; break;
                case 2001: gameUI.currentState = STATE_GAMEPLAY; break;
                case 2010: { 
                    jclass actCls = env->GetObjectClass(obj);
                    jmethodID purchaseMid = env->GetMethodID(actCls, "dispatchRemoveAdsPurchase", "()V");
                    if (purchaseMid) env->CallVoidMethod(obj, purchaseMid);
                    break;
                }
                case 3500:
                    for(int i = 0; i < 50; i++) gameUI.levelsUnlocked[i] = true;
                    gameUI.currentState = STATE_LEVELS;
                    break;
                case 4001:
                    if (gameUI.currentPlayingLevel < 50) {
                        gameUI.levelsUnlocked[gameUI.currentPlayingLevel] = true;
                        gameUI.currentPlayingLevel++;
                    }
                    gameUI.currentState = STATE_LEVELS;
                    break;
                case 4002: gameUI.isPausePopupActive = true; break;
                case 4003: gameUI.isHintPopupActive = true; break;
                case 6501: gameUI.isThemePopupActive = true; break;
                case 6504: 
                    gameUI.selectedRatingStars = 0;
                    gameUI.isRatingPopupActive = true; 
                    break;
                case 6502: {
                    jclass actCls = env->GetObjectClass(obj);
                    jmethodID midIntent = env->GetMethodID(actCls, "dispatchPrivacyPolicyIntent", "()V");
                    if (midIntent) env->CallVoidMethod(obj, midIntent);
                    break;
                }
                case 6503: {
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
