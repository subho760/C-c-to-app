#include "game_structures.h"
#include <algorithm>
#include <cmath>

// Helper to toggle font weight for text rendering
void setPaintFontWeight(JNIEnv* env, jobject paint, bool bold) {
    if (!paint) return;
    jclass paintCls = env->GetObjectClass(paint);
    jmethodID setFakeBoldText = env->GetMethodID(paintCls, "setFakeBoldText", "(Z)V");
    if (setFakeBoldText) env->CallVoidMethod(paint, setFakeBoldText, bold);
}

// ---------------------------------------------------------------------------
// 1. MAIN HEADER ("rrows" title + animated/rotated arrow)
// ---------------------------------------------------------------------------
void drawGameHeader(JNIEnv* env, jobject obj, jobject canvas, int baseBgColor, int baseTxtColor) {
    if (!gameUI.paintTextReference) return;

    setPaintFontWeight(env, gameUI.paintTextReference, true);
    jclass paintCls = env->GetObjectClass(gameUI.paintTextReference);
    jmethodID setTextSize = env->GetMethodID(paintCls, "setTextSize", "(F)V");
    jmethodID setColor = env->GetMethodID(paintCls, "setColor", "(I)V");
    jmethodID measureText = env->GetMethodID(paintCls, "measureText", "(Ljava/lang/String;)F");

    env->CallVoidMethod(gameUI.paintTextReference, setTextSize, 72.0f);
    env->CallVoidMethod(gameUI.paintTextReference, setColor, baseTxtColor);

    float midPointX = gameUI.screenWidth / 2.0f;
    // Positioned nicely near top of screen
    float fixedHeaderY = gameUI.screenHeight * 0.20f; 

    jstring rrowStr = env->NewStringUTF("rrows");
    float rrowWidth = env->CallFloatMethod(gameUI.paintTextReference, measureText, rrowStr);

    float inlineArrowSize = 60.0f;
    float totalHeaderWidth = inlineArrowSize + rrowWidth + 8.0f;
    float startX = midPointX - (totalHeaderWidth / 2.0f);
    float arrowY = fixedHeaderY - 56.0f;

    if (gameUI.assetBitmaps[ASSET_ARROW]) {
        env->CallIntMethod(canvas, gameUI.midSave);
        jmethodID midRotate = env->GetMethodID(env->GetObjectClass(canvas), "rotate", "(FFF)V");
        env->CallVoidMethod(canvas, midRotate, -90.0f, startX + (inlineArrowSize / 2.0f), arrowY + (inlineArrowSize / 2.0f));
        renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_ARROW], startX, arrowY, inlineArrowSize, inlineArrowSize);
        env->CallVoidMethod(canvas, gameUI.midRestore);
    }

    float rrowX = startX + inlineArrowSize + 8.0f;
    env->CallVoidMethod(canvas, gameUI.midDrawText, rrowStr, rrowX, fixedHeaderY, gameUI.paintTextReference);
    env->DeleteLocalRef(rrowStr);
    setPaintFontWeight(env, gameUI.paintTextReference, false);
}

// ---------------------------------------------------------------------------
// 2. BOTTOM NAVIGATION BAR
// ---------------------------------------------------------------------------
void drawBottomNavigationBar(JNIEnv* env, jobject obj, jobject canvas) {
    float barHeight = 80.0f;
    float barY = gameUI.screenHeight - barHeight;
    float sectionWidth = gameUI.screenWidth / 3.0f;
    float iconSize = 40.0f;
    float iconY = barY + (barHeight - iconSize) / 2.0f;

    // Draw background bar
    drawRoundRectNative(env, canvas, 0, barY, gameUI.screenWidth, gameUI.screenHeight, 0, 0, gameUI.isCurrentlyDark ? 0xFF181A20 : 0xFFEFEFEF);

    // Tab 0: Home
    float homeX = (sectionWidth - iconSize) / 2.0f;
    if (gameUI.assetBitmaps[ASSET_HOME]) {
        renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_HOME], homeX, iconY, iconSize, iconSize);
    }
    gameUI.UIButtons.push_back({0, barY, sectionWidth, barHeight, 6001, 0}); // Action: Go Home

    // Tab 1: Level Select
    float levelX = sectionWidth + (sectionWidth - iconSize) / 2.0f;
    if (gameUI.assetBitmaps[ASSET_LEVELS]) {
        renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_LEVELS], levelX, iconY, iconSize, iconSize);
    }
    gameUI.UIButtons.push_back({sectionWidth, barY, sectionWidth, barHeight, 6002, 0}); // Action: Go Levels

    // Tab 2: Settings / Theme
    float settingsX = (sectionWidth * 2.0f) + (sectionWidth - iconSize) / 2.0f;
    if (gameUI.assetBitmaps[ASSET_SETTINGS]) {
        renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_SETTINGS], settingsX, iconY, iconSize, iconSize);
    }
    gameUI.UIButtons.push_back({sectionWidth * 2.0f, barY, sectionWidth, barHeight, 6003, 0}); // Action: Go Settings
}

// ---------------------------------------------------------------------------
// 3. HOME SCREEN
// ---------------------------------------------------------------------------
void drawHomeScreen(JNIEnv* env, jobject obj, jobject canvas) {
    if (gameUI.currentState != STATE_HOME) return;

    // Remove Ads Icon in Top Right Corner
    if (gameUI.assetBitmaps[ASSET_REMOVE_ADS]) {
        float iconSize = 70.0f;
        float padding = 32.0f;
        float removeAdsX = gameUI.screenWidth - iconSize - padding;
        float removeAdsY = padding;

        renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_REMOVE_ADS], removeAdsX, removeAdsY, iconSize, iconSize);
        gameUI.UIButtons.push_back({removeAdsX, removeAdsY, iconSize, iconSize, 8888, 0});
    }

    // Title Header
    drawGameHeader(env, obj, canvas, 0, 0xFFFFFFFF);

    // Subtitle ("Level 1" indicator)
    if (gameUI.paintTextReference) {
        jclass paintCls = env->GetObjectClass(gameUI.paintTextReference);
        jmethodID setTextSize = env->GetMethodID(paintCls, "setTextSize", "(F)V");
        jmethodID setColor = env->GetMethodID(paintCls, "setColor", "(I)V");
        
        env->CallVoidMethod(gameUI.paintTextReference, setTextSize, 40.0f);
        env->CallVoidMethod(gameUI.paintTextReference, setColor, 0xFF5B6EFF);

        jstring subStr = env->NewStringUTF("Level 1");
        float midX = gameUI.screenWidth / 2.0f - 60.0f;
        float subY = gameUI.screenHeight * 0.25f;
        env->CallVoidMethod(canvas, gameUI.midDrawText, subStr, midX, subY, gameUI.paintTextReference);
        env->DeleteLocalRef(subStr);
    }

    // Play Button (Raised up to 0.75f to clear bottom nav)
    float playBtnW = gameUI.screenWidth * 0.80f;
    float playBtnH = 60.0f;
    float playBtnX = (gameUI.screenWidth - playBtnW) / 2.0f;
    float playBtnY = gameUI.screenHeight * 0.75f; 

    drawRoundRectNative(env, canvas, playBtnX, playBtnY, playBtnX + playBtnW, playBtnY + playBtnH, 30, 30, 0xFF5B6EFF);
    gameUI.UIButtons.push_back({playBtnX, playBtnY, playBtnW, playBtnH, 1001, 0});
}

// ---------------------------------------------------------------------------
// 4. LEVEL SELECTION SCREEN
// ---------------------------------------------------------------------------
void renderLevelSelectionGrid(JNIEnv* env, jobject canvas) {
    if (gameUI.currentState != STATE_LEVELS) return;

    int totalLevels = 18;
    int cols = 3;
    float cardSize = gameUI.screenWidth * 0.25f;
    float spacing = gameUI.screenWidth * 0.05f;
    
    float totalGridWidth = (cols * cardSize) + ((cols - 1) * spacing);
    float startX = (gameUI.screenWidth - totalGridWidth) / 2.0f;
    float startY = gameUI.screenHeight * 0.15f;

    // Calculate maximum dynamic scroll distance
    int rows = std::ceil((float)totalLevels / cols);
    float totalGridHeight = (rows * cardSize) + ((rows - 1) * spacing);
    float visibleAreaHeight = gameUI.screenHeight * 0.70f;
    
    gameUI.maxScrollExtent = std::max(0.0f, totalGridHeight - visibleAreaHeight);

    for (int i = 0; i < totalLevels; ++i) {
        int row = i / cols;
        int col = i % cols;

        float btnX = startX + col * (cardSize + spacing);
        float btnY = startY + row * (cardSize + spacing) + gameUI.levelScrollOffset;

        // Viewport constraint: restrict touches & rendering from bleeding into top header or bottom nav
        if (btnY >= (gameUI.screenHeight * 0.10f) && (btnY + cardSize) <= (gameUI.screenHeight * 0.82f)) {
            
            int cardBg = (i == 0) ? 0xFF5B6EFF : (i == 1 ? 0xFFF2994A : 0xFF252A34);
            drawRoundRectNative(env, canvas, btnX, btnY, btnX + cardSize, btnY + cardSize, 20, 20, cardBg);

            // Level Button Action Code
            gameUI.UIButtons.push_back({btnX, btnY, cardSize, cardSize, 7000 + i, i});
        }
    }
}

// ---------------------------------------------------------------------------
// 5. OVERLAYS & POPUPS (Pause, Hint, Theme, Rating)
// ---------------------------------------------------------------------------
void drawPopupsAndOverlays(JNIEnv* env, jobject canvas) {
    // Dimmed background backdrop when any popup is active
    if (gameUI.isHintPopupActive || gameUI.isThemePopupActive || gameUI.isRatingPopupActive || gameUI.isPausePopupActive) {
        drawRoundRectNative(env, canvas, 0, 0, gameUI.screenWidth, gameUI.screenHeight, 0, 0, 0xAA000000);
        
        // Fullscreen dismiss button registered on active overlay
        gameUI.UIButtons.push_back({0, 0, gameUI.screenWidth, gameUI.screenHeight, 9999, 0});
    }
}

// ---------------------------------------------------------------------------
// 6. MASTER RENDER DISPATCHER
// ---------------------------------------------------------------------------
void renderUI(JNIEnv* env, jobject obj, jobject canvas) {
    // Clear interactive UI buttons for current frame
    gameUI.UIButtons.clear();

    switch (gameUI.currentState) {
        case STATE_HOME:
            drawHomeScreen(env, obj, canvas);
            break;
        case STATE_LEVELS:
            renderLevelSelectionGrid(env, canvas);
            break;
        default:
            break;
    }

    // Always draw bottom nav bar
    drawBottomNavigationBar(env, obj, canvas);

    // Draw active popups over screen
    drawPopupsAndOverlays(env, canvas);
}
