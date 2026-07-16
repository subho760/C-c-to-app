#ifndef GAME_STRUCTURES_H
#define GAME_STRUCTURES_H

#include <jni.h>
#include <string>
#include <vector>
#include <cmath>

enum GameState { STATE_LOADING, STATE_HOME, STATE_SETTINGS, STATE_GAMEPLAY, STATE_LEVELS };
enum ThemeMode { THEME_SYSTEM, THEME_LIGHT, THEME_DARK };

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

    ThemeMode activeTheme = THEME_SYSTEM;
    bool isCurrentlyDark = false;

    bool levelsUnlocked[50] = { true, false };

    float levelScrollOffset = 0.0f;
    float maxScrollExtent = 0.0f;

    bool isHintPopupActive = false;
    bool isThemePopupActive = false;
    bool isRatingPopupActive = false;
    bool isPausePopupActive = false;

    int selectedRatingStars = 0;

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
    jmethodID midDrawCircle = nullptr;
    jmethodID midGetWidth = nullptr;
    jmethodID midGetHeight = nullptr;

    jobject paintTextReference = nullptr;
    jobject paintShapeReference = nullptr;
};

extern GameMenuStructure gameUI;

// Shared Helper Functions (Marked extern so any C++ file linking them finds them perfectly)
extern void renderBmp(JNIEnv* env, jobject canvas, jobject bitmap, float leftX, float topY, float forcedWidth, jobject customPaint = nullptr);
extern void drawRoundRectNative(JNIEnv* env, jobject canvas, float left, float top, float right, float bottom, float rx, float ry, int colorHex);
extern void drawRealShadowRoundRect(JNIEnv* env, jobject canvas, float left, float top, float right, float bottom, float rx, float ry);
extern void setPaintFontWeight(JNIEnv* env, jobject paintRef, bool isBold);
extern jobject getTintPaint(JNIEnv* env, jobject obj, int colorHex);
extern int getNextUnlockableLevel();

// UI Layer Drawing Functions
extern void drawGameHeader(JNIEnv* env, jobject obj, jobject canvas, int baseBgColor, int baseTxtColor, jobject tintRed);
extern void drawWatermark(JNIEnv* env, jobject canvas);
extern void drawHorizontalPausePopup(JNIEnv* env, jobject canvas, float dX, float dY, float dW, float dH, jobject tintActive);
extern void checkGlobalClosePopupDismiss(float touchX, float touchY);

#endif
