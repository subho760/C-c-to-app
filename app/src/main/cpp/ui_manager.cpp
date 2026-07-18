#include "game_structures.h"
#include <cmath>
#include <string>
#include <algorithm>

// Premium Modern UI Palette Constants
#define COLOR_DARK_BG       0xFF161A23  
#define COLOR_LIGHT_BG      0xFFF5F7FA  
#define COLOR_DARK_CARD     0xFF222A3A  
#define COLOR_LIGHT_CARD    0xFFFFFFFF  
#define COLOR_ACCENT_BLUE   0xFF5773FF  
#define COLOR_TEXT_MUTED    0xFF7E8B9B  
#define COLOR_VIVID_RED     0xFFFF3B30  

static int nativeRatingScore = 0; 
static float gameplayZoomScale = 1.0f; 
static bool localIsAdWatchPopupActive = false; 

// Internal tracker for targeting popups explicitly
static int pendingUnlockLevelIndex = -1;

void drawDialogButton(JNIEnv* env, jobject canvas, float x, float y, float w, float h, const char* label, int bgColor, int textColor) {
    float cornerRadius = h / 2.0f;
    drawRoundRectNative(env, canvas, x, y, x + w, y + h, cornerRadius, cornerRadius, bgColor);
    
    if (gameUI.paintTextReference) {
        setPaintFontWeight(env, gameUI.paintTextReference, true);
        jclass paintCls = env->GetObjectClass(gameUI.paintTextReference);
        jmethodID setTextSize = env->GetMethodID(paintCls, "setTextSize", "(F)V");
        jmethodID setColor = env->GetMethodID(paintCls, "setColor", "(I)V");
        
        env->CallVoidMethod(gameUI.paintTextReference, setTextSize, 32.0f);
        env->CallVoidMethod(gameUI.paintTextReference, setColor, textColor);
        
        jstring jText = env->NewStringUTF(label);
        jmethodID measureText = env->GetMethodID(paintCls, "measureText", "(Ljava/lang/String;)F");
        float textW = env->CallFloatMethod(gameUI.paintTextReference, measureText, jText);
        
        float tx = x + (w - textW) / 2.0f;
        float ty = y + (h / 2.0f) + 10.0f; 
        env->CallVoidMethod(canvas, gameUI.midDrawText, jText, tx, ty, gameUI.paintTextReference);
        env->DeleteLocalRef(jText);
        setPaintFontWeight(env, gameUI.paintTextReference, false);
    }
}

void drawGameHeader(JNIEnv* env, jobject obj, jobject canvas, int baseBgColor, int baseTxtColor, jobject tintRed) {
    if (!gameUI.paintTextReference) return;
    
    setPaintFontWeight(env, gameUI.paintTextReference, true);
    jclass paintCls = env->GetObjectClass(gameUI.paintTextReference);
    jmethodID setTextSize = env->GetMethodID(paintCls, "setTextSize", "(F)V");
    jmethodID setColor = env->GetMethodID(paintCls, "setColor", "(I)V");
    jmethodID measureText = env->GetMethodID(paintCls, "measureText", "(Ljava/lang/String;)F");
    
    env->CallVoidMethod(gameUI.paintTextReference, setTextSize, 72.0f);
    env->CallVoidMethod(gameUI.paintTextReference, setColor, baseTxtColor);

    float midPointX = gameUI.screenWidth / 2.0f;
    float fixedHeaderY = gameUI.screenHeight * 0.48f; 

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
        renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_ARROW], startX, arrowY, inlineArrowSize, tintRed);
        env->CallVoidMethod(canvas, gameUI.midRestore);
    }

    float rrowX = startX + inlineArrowSize + 8.0f;
    env->CallVoidMethod(canvas, gameUI.midDrawText, rrowStr, rrowX, fixedHeaderY, gameUI.paintTextReference);
    env->DeleteLocalRef(rrowStr);
    setPaintFontWeight(env, gameUI.paintTextReference, false);
}

void drawHorizontalPausePopup(JNIEnv* env, jobject canvas, float dX, float dY, float dW, float dH, jobject tintActive) {
    drawRoundRectNative(env, canvas, dX, dY, dX + dW, dY + dH, 44, 44, gameUI.isCurrentlyDark ? COLOR_DARK_CARD : COLOR_LIGHT_CARD);

    float buttonSize = dW * 0.16f; 
    float innerSpacingY = dY + (dH / 2.0f) - (buttonSize / 2.0f);
    float itemHorizontalStep = dW / 4.0f;

    float btn1X = dX + itemHorizontalStep - (buttonSize / 2.0f);
    if (gameUI.assetBitmaps[ASSET_PLAY]) {
        renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_PLAY], btn1X, innerSpacingY, buttonSize, tintActive);
    }
    gameUI.UIButtons.push_back({btn1X, innerSpacingY, buttonSize, buttonSize, 5501, 0});

    float btn2X = dX + (itemHorizontalStep * 2.0f) - (buttonSize / 2.0f);
    if (gameUI.assetBitmaps[ASSET_RETRY]) {
        renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_RETRY], btn2X, innerSpacingY, buttonSize, tintActive);
    }
    gameUI.UIButtons.push_back({btn2X, innerSpacingY, buttonSize, buttonSize, 5502, 0});

    float btn3X = dX + (itemHorizontalStep * 3.0f) - (buttonSize / 2.0f);
    if (gameUI.assetBitmaps[ASSET_HOME]) {
        renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_HOME], btn3X, innerSpacingY, buttonSize, tintActive);
    }
    gameUI.UIButtons.push_back({btn3X, innerSpacingY, buttonSize, buttonSize, 5504, 0});
}

extern "C" {

JNIEXPORT void JNICALL
Java_com_night_backgroundchange_MainActivity_nativeRender(JNIEnv* env, jobject obj, jobject canvas) {
    if (!canvas || !gameUI.engineInitialized || !gameUI.midDrawColor) return;

    int baseBgColor = gameUI.isCurrentlyDark ? COLOR_DARK_BG : COLOR_LIGHT_BG;
    int baseCardColor = gameUI.isCurrentlyDark ? COLOR_DARK_CARD : COLOR_LIGHT_CARD;
    int baseTxtColor = gameUI.isCurrentlyDark ? 0xFFFFFFFF : 0xFF1A1F26;
    int activeSelectionColor = COLOR_ACCENT_BLUE;
    int unselectedGrayColor = COLOR_TEXT_MUTED;

    env->CallVoidMethod(canvas, gameUI.midDrawColor, baseBgColor);
    gameUI.UIButtons.clear();

    jobject tintActive = getTintPaint(env, obj, activeSelectionColor);
    jobject tintGray = getTintPaint(env, obj, unselectedGrayColor);
    jobject tintWhite = getTintPaint(env, obj, 0xFFFFFFFF);
    jobject tintYellow = getTintPaint(env, obj, 0xFFFFCC00);
    jobject tintVividRed = getTintPaint(env, obj, COLOR_VIVID_RED);

    float headerFixedBarHeight = 160.0f;
    float footerFixedBarHeight = 160.0f;
    float footerStartY = gameUI.screenHeight - footerFixedBarHeight - 60.0f;

    bool activeModalBlocks = (gameUI.isHintPopupActive || gameUI.isThemePopupActive || gameUI.isRatingPopupActive || gameUI.isPausePopupActive || localIsAdWatchPopupActive);

    // --- 1. HOME SCREEN ---
    if (gameUI.currentState == STATE_HOME && !activeModalBlocks) {
        drawGameHeader(env, obj, canvas, baseBgColor, baseTxtColor, tintVividRed);

        if (gameUI.paintTextReference) {
            jclass paintCls = env->GetObjectClass(gameUI.paintTextReference);
            jmethodID setTextSize = env->GetMethodID(paintCls, "setTextSize", "(F)V");
            jmethodID setColor = env->GetMethodID(paintCls, "setColor", "(I)V");
            jmethodID measureText = env->GetMethodID(paintCls, "measureText", "(Ljava/lang/String;)F");
            
            setPaintFontWeight(env, gameUI.paintTextReference, true);
            env->CallVoidMethod(gameUI.paintTextReference, setTextSize, 42.0f);
            env->CallVoidMethod(gameUI.paintTextReference, setColor, COLOR_ACCENT_BLUE);
            
            std::string currentLvlInfo = "Level " + std::to_string(gameUI.currentPlayingLevel > 0 ? gameUI.currentPlayingLevel : 1);
            jstring jLvlStr = env->NewStringUTF(currentLvlInfo.c_str());
            float lvlW = env->CallFloatMethod(gameUI.paintTextReference, measureText, jLvlStr);
            env->CallVoidMethod(canvas, gameUI.midDrawText, jLvlStr, (gameUI.screenWidth - lvlW) / 2.0f, (gameUI.screenHeight * 0.48f) + 65.0f, gameUI.paintTextReference);
            env->DeleteLocalRef(jLvlStr);
            setPaintFontWeight(env, gameUI.paintTextReference, false);
        }

        float playW = gameUI.screenWidth * 0.68f;
        float playH = 120.0f;
        float playX = (gameUI.screenWidth - playW) / 2.0f;
        float playY = footerStartY - 100.0f;

        drawDialogButton(env, canvas, playX, playY, playW, playH, "Play", COLOR_ACCENT_BLUE, 0xFFFFFFFF);
        gameUI.UIButtons.push_back({playX, playY, playW, playH, 2001, 0});
    }

    // --- 2. SELECT LEVEL SCREEN ---
    if (gameUI.currentState == STATE_LEVELS && !activeModalBlocks) {
        float boxSize = gameUI.screenWidth * 0.25f;
        float spaceGrid = gameUI.screenWidth * 0.04f;
        float offsetGridX = (gameUI.screenWidth - (3 * boxSize + 2 * spaceGrid)) / 2.0f;
        
        int totalRows = (int)std::ceil(50.0f / 3.0f);
        float extensionButtonHeight = 90.0f;

        env->CallIntMethod(canvas, gameUI.midSave);
        
        float lastRowBottomY = 0.0f;
        for (int i = 0; i < 50; i++) {
            int row = i / 3;
            int col = i % 3;
            float bx = offsetGridX + col * (boxSize + spaceGrid);
            float by = headerFixedBarHeight + 30.0f + row * (boxSize + spaceGrid) + gameUI.levelScrollOffset;
            
            float currentBottom = by + boxSize;
            if (currentBottom > lastRowBottomY) {
                lastRowBottomY = currentBottom;
            }

            if (by + boxSize < headerFixedBarHeight || by > footerStartY) continue;

            int finalBoxColor = baseCardColor; 
            int textLvlColor = baseTxtColor;
            
            bool isAdWatchLevel = (i > 0 && gameUI.levelsUnlocked[i - 1] && !gameUI.levelsUnlocked[i]);
            
            if (gameUI.levelsUnlocked[i]) {
                if (i == gameUI.currentPlayingLevel - 1 || (gameUI.currentPlayingLevel == 0 && i == 0)) {
                    finalBoxColor = COLOR_ACCENT_BLUE;
                    textLvlColor = 0xFFFFFFFF;
                }
            } else if (isAdWatchLevel) {
                finalBoxColor = 0xFFE58E26; // Orange visual state
            }
            
            drawRoundRectNative(env, canvas, bx, by, bx + boxSize, by + boxSize, 24, 24, finalBoxColor);

            if (!gameUI.levelsUnlocked[i]) {
                float iconSz = boxSize * 0.35f;
                float iconOffset = (boxSize - iconSz) / 2.0f;
                if (isAdWatchLevel) {
                    if (gameUI.assetBitmaps[ASSET_REMOVE_ADS]) {
                        renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_REMOVE_ADS], bx + iconOffset, by + iconOffset, iconSz, tintWhite);
                    }
                } else {
                    if (gameUI.assetBitmaps[ASSET_LOCK]) {
                        renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_LOCK], bx + iconOffset, by + iconOffset, iconSz, tintGray);
                    }
                }
            } else {
                setPaintFontWeight(env, gameUI.paintTextReference, true);
                if (gameUI.paintTextReference) {
                    jclass paintCls = env->GetObjectClass(gameUI.paintTextReference);
                    jmethodID setTextSize = env->GetMethodID(paintCls, "setTextSize", "(F)V");
                    jmethodID setColor = env->GetMethodID(paintCls, "setColor", "(I)V");
                    env->CallVoidMethod(gameUI.paintTextReference, setTextSize, 38.0f);
                    env->CallVoidMethod(gameUI.paintTextReference, setColor, textLvlColor);
                }
                std::string numLvl = std::to_string(i + 1);
                jstring jNumL = env->NewStringUTF(numLvl.c_str());
                
                jclass paintCls = env->GetObjectClass(gameUI.paintTextReference);
                jmethodID measureText = env->GetMethodID(paintCls, "measureText", "(Ljava/lang/String;)F");
                float numW = env->CallFloatMethod(gameUI.paintTextReference, measureText, jNumL);
                
                env->CallVoidMethod(canvas, gameUI.midDrawText, jNumL, bx + (boxSize - numW) / 2.0f, by + (boxSize / 2.0f) + 13.0f, gameUI.paintTextReference);
                env->DeleteLocalRef(jNumL);
                setPaintFontWeight(env, gameUI.paintTextReference, false);
            }

            int interactionCode = 4150; 
            if (gameUI.levelsUnlocked[i]) {
                interactionCode = 3000 + i;
            } else if (isAdWatchLevel) {
                interactionCode = 7800 + i; 
            }
            gameUI.UIButtons.push_back({bx, by, boxSize, boxSize, interactionCode, i});
        }

        float extensionY = lastRowBottomY + 40.0f;
        if (extensionY + extensionButtonHeight >= headerFixedBarHeight && extensionY <= footerStartY) {
            float extW = gameUI.screenWidth - (2.0f * offsetGridX);
            drawDialogButton(env, canvas, offsetGridX, extensionY, extW, extensionButtonHeight, "Complete all levels to open more", baseCardColor, unselectedGrayColor);
            gameUI.UIButtons.push_back({offsetGridX, extensionY, extW, extensionButtonHeight, 3999, 0});
        }

        env->CallVoidMethod(canvas, gameUI.midRestore);

        // Header Base Backdrop Mask
        drawRoundRectNative(env, canvas, 0, 0, gameUI.screenWidth, headerFixedBarHeight, 0, 0, baseBgColor);
        setPaintFontWeight(env, gameUI.paintTextReference, true);
        if (gameUI.paintTextReference) {
            jclass paintCls = env->GetObjectClass(gameUI.paintTextReference);
            jmethodID setTextSize = env->GetMethodID(paintCls, "setTextSize", "(F)V");
            env->CallVoidMethod(gameUI.paintTextReference, setTextSize, 46.0f);
        }
        
        jstring levelHeader = env->NewStringUTF("SELECT LEVEL");
        jclass paintCls = env->GetObjectClass(gameUI.paintTextReference);
        jmethodID measureText = env->GetMethodID(paintCls, "measureText", "(Ljava/lang/String;)F");
        float headerTextW = env->CallFloatMethod(gameUI.paintTextReference, measureText, levelHeader);
        env->CallVoidMethod(canvas, gameUI.midDrawText, levelHeader, (gameUI.screenWidth / 2.0f) - (headerTextW / 2.0f), 105.0f, gameUI.paintTextReference);
        env->DeleteLocalRef(levelHeader);
        setPaintFontWeight(env, gameUI.paintTextReference, false);
    }

    // --- 3. SETTINGS MENU VIEW ---
    if (gameUI.currentState == STATE_SETTINGS && !activeModalBlocks) {
        drawRoundRectNative(env, canvas, 0, 0, gameUI.screenWidth, headerFixedBarHeight, 0, 0, baseBgColor);
        if (gameUI.paintTextReference) {
            setPaintFontWeight(env, gameUI.paintTextReference, true);
            jclass paintCls = env->GetObjectClass(gameUI.paintTextReference);
            jmethodID setTextSize = env->GetMethodID(paintCls, "setTextSize", "(F)V");
            env->CallVoidMethod(gameUI.paintTextReference, setTextSize, 46.0f);
            jstring setHeader = env->NewStringUTF("SETTINGS");
            env->CallVoidMethod(canvas, gameUI.midDrawText, setHeader, 45.0f, 105.0f, gameUI.paintTextReference);
            env->DeleteLocalRef(setHeader);
            setPaintFontWeight(env, gameUI.paintTextReference, false);
        }

        float optionY = headerFixedBarHeight + 30.0f;
        float optionHeight = 110.0f; 
        float optionSpacing = 20.0f;
        float marginX = 45.0f;
        float rowWidth = gameUI.screenWidth - (2 * marginX);

        const char* optionsNames[] = {"Dark mode", "Rate us", "Share app", "Privacy"};
        int optionActions[] = {6501, 6504, 6503, 6502}; 

        for (int i = 0; i < 4; i++) {
            drawRoundRectNative(env, canvas, marginX, optionY, marginX + rowWidth, optionY + optionHeight, 24, 24, baseCardColor);

            if (gameUI.paintTextReference) {
                jclass paintCls = env->GetObjectClass(gameUI.paintTextReference);
                jmethodID setTextSize = env->GetMethodID(paintCls, "setTextSize", "(F)V");
                jmethodID setColor = env->GetMethodID(paintCls, "setColor", "(I)V");
                env->CallVoidMethod(gameUI.paintTextReference, setTextSize, 36.0f);
                env->CallVoidMethod(gameUI.paintTextReference, setColor, baseTxtColor);
                
                jstring textStr = env->NewStringUTF(optionsNames[i]);
                env->CallVoidMethod(canvas, gameUI.midDrawText, textStr, marginX + 40.0f, optionY + 68.0f, gameUI.paintTextReference);
                env->DeleteLocalRef(textStr);
            }

            if (i == 0) {
                float swW = 85.0f;
                float swH = 46.0f;
                float swX = marginX + rowWidth - swW - 40.0f;
                float swY = optionY + (optionHeight - swH) / 2.0f;
                drawRoundRectNative(env, canvas, swX, swY, swX + swW, swY + swH, swH/2.0f, swH/2.0f, gameUI.isCurrentlyDark ? COLOR_ACCENT_BLUE : unselectedGrayColor);
                drawRoundRectNative(env, canvas, gameUI.isCurrentlyDark ? (swX + swW - 40.0f) : (swX + 6.0f), swY + 5.0f, gameUI.isCurrentlyDark ? (swX + swW - 6.0f) : (swX + swH - 11.0f), swY + swH - 5.0f, 18, 18, 0xFFFFFFFF);
            }
            
            if (i == 1 && gameUI.assetBitmaps[ASSET_STAR]) {
                float starIconSz = 40.0f;
                float starIconX = marginX + rowWidth - starIconSz - 40.0f;
                float starIconY = optionY + (optionHeight - starIconSz) / 2.0f;
                renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_STAR], starIconX, starIconY, starIconSz, tintYellow);
            }

            gameUI.UIButtons.push_back({marginX, optionY, rowWidth, optionHeight, optionActions[i], 0});
            optionY += optionHeight + optionSpacing;
        }
    }

    // --- 4. GAMEPLAY SCREEN ---
    if (gameUI.currentState == STATE_GAMEPLAY && !activeModalBlocks) {
        float headerIconSize = 65.0f;
        float baseIconY = 45.0f;

        drawRoundRectNative(env, canvas, 0, 0, gameUI.screenWidth, headerFixedBarHeight, 0, 0, baseBgColor);

        if (gameUI.assetBitmaps[ASSET_PAUSED]) {
            renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_PAUSED], 40.0f, baseIconY, headerIconSize, tintWhite);
            gameUI.UIButtons.push_back({40.0f, baseIconY, headerIconSize, headerIconSize, 4002, 0});
        }

        if (gameUI.assetBitmaps[ASSET_HINT]) {
            float hintX = gameUI.screenWidth - headerIconSize - 40.0f;
            renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_HINT], hintX, baseIconY, headerIconSize, tintWhite);
            gameUI.UIButtons.push_back({hintX, baseIconY, headerIconSize, headerIconSize, 4003, 0});
        }

        if (gameUI.paintTextReference) {
            setPaintFontWeight(env, gameUI.paintTextReference, true);
            jclass paintCls = env->GetObjectClass(gameUI.paintTextReference);
            jmethodID setTextSize = env->GetMethodID(paintCls, "setTextSize", "(F)V");
            jmethodID setColor = env->GetMethodID(paintCls, "setColor", "(I)V");
            jmethodID measureText = env->GetMethodID(paintCls, "measureText", "(Ljava/lang/String;)F");

            env->CallVoidMethod(gameUI.paintTextReference, setTextSize, 42.0f);
            env->CallVoidMethod(gameUI.paintTextReference, setColor, baseTxtColor);

            std::string lvlBanner = "Level " + std::to_string(gameUI.currentPlayingLevel);
            jstring jBanner = env->NewStringUTF(lvlBanner.c_str());
            float textW = env->CallFloatMethod(gameUI.paintTextReference, measureText, jBanner);
            env->CallVoidMethod(canvas, gameUI.midDrawText, jBanner, (gameUI.screenWidth / 2.0f) - (textW / 2.0f), 95.0f, gameUI.paintTextReference);
            env->DeleteLocalRef(jBanner);
            setPaintFontWeight(env, gameUI.paintTextReference, false);
        }

        env->CallIntMethod(canvas, gameUI.midSave);
        jmethodID midScale = env->GetMethodID(env->GetObjectClass(canvas), "scale", "(FFFF)V");
        env->CallVoidMethod(canvas, midScale, gameplayZoomScale, gameplayZoomScale, gameUI.screenWidth / 2.0f, gameUI.screenHeight / 2.0f);

        int gridColumns = 5;
        int gridRows = 10;
        float startGridY = headerFixedBarHeight + 80.0f;
        float availableGridHeight = footerStartY - startGridY - 40.0f;
        float stepX = gameUI.screenWidth / (float)(gridColumns + 1);
        float stepY = availableGridHeight / (float)(gridRows + 1);

        float nodeCoordinatesX[50];
        float nodeCoordinatesY[50];

        jclass paintClass = env->FindClass("android/graphics/Paint");
        jmethodID paintInit = env->GetMethodID(paintClass, "<init>", "()V");
        jobject dotPaint = env->NewObject(paintClass, paintInit);
        jmethodID setPaintColorMethod = env->GetMethodID(paintClass, "setColor", "(I)V");
        jmethodID setAlphaMethod = env->GetMethodID(paintClass, "setAlpha", "(I)V");

        env->CallVoidMethod(dotPaint, setPaintColorMethod, gameUI.isCurrentlyDark ? 0xFFFFFFFF : 0xFF222222);
        env->CallVoidMethod(dotPaint, setAlphaMethod, 40);

        jmethodID midDrawCircle = env->GetMethodID(env->GetObjectClass(canvas), "drawCircle", "(FFFLandroid/graphics/Paint;)V");

        int dotIndex = 0;
        for (int r = 1; r <= gridRows; r++) {
            for (int c = 1; c <= gridColumns; c++) {
                float circleX = c * stepX;
                float circleY = startGridY + (r * stepY);
                nodeCoordinatesX[dotIndex] = circleX;
                nodeCoordinatesY[dotIndex] = circleY;
                env->CallVoidMethod(canvas, midDrawCircle, circleX, circleY, 8.0f, dotPaint);
                dotIndex++;
            }
        }

        jobject linePaint = env->NewObject(paintClass, paintInit);
        jmethodID setStrokeWidth = env->GetMethodID(paintClass, "setStrokeWidth", "(F)V");
        env->CallVoidMethod(linePaint, setPaintColorMethod, COLOR_ACCENT_BLUE); 
        env->CallVoidMethod(linePaint, setStrokeWidth, 8.0f);
        jmethodID midDrawLine = env->GetMethodID(env->GetObjectClass(canvas), "drawLine", "(FFFFLandroid/graphics/Paint;)V");

        float nodeStartX = nodeCoordinatesX[0];
        float nodeStartY = nodeCoordinatesY[0];
        float nodeEndX = nodeCoordinatesX[49];
        float nodeEndY = nodeCoordinatesY[49];

        float arrowSizeOffset = 30.0f; 
        env->CallVoidMethod(canvas, midDrawLine, nodeStartX, nodeStartY, nodeEndX, nodeStartY, linePaint);
        env->CallVoidMethod(canvas, midDrawLine, nodeEndX, nodeStartY, nodeEndX, nodeEndY - arrowSizeOffset, linePaint);

        if (gameUI.assetBitmaps[ASSET_ARROW]) {
            env->CallIntMethod(canvas, gameUI.midSave);
            jmethodID midRotate = env->GetMethodID(env->GetObjectClass(canvas), "rotate", "(FFF)V");
            env->CallVoidMethod(canvas, midRotate, 180.0f, nodeEndX, nodeEndY);
            renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_ARROW], nodeEndX - 25.0f, nodeEndY - 25.0f, 50.0f, tintWhite);
            env->CallVoidMethod(canvas, gameUI.midRestore);
        }

        env->CallVoidMethod(canvas, gameUI.midRestore);
        env->DeleteLocalRef(dotPaint);
        env->DeleteLocalRef(linePaint);
        env->DeleteLocalRef(paintClass);
    }

    // --- 5. FIXED LAYER BOTTOM NAVIGATION VIEW BAR ---
    if (gameUI.currentState == STATE_HOME || gameUI.currentState == STATE_SETTINGS || gameUI.currentState == STATE_LEVELS) {
        drawRoundRectNative(env, canvas, 0, footerStartY, gameUI.screenWidth, gameUI.screenHeight, 0, 0, baseCardColor);

        float navIconSize = 55.0f; 
        float innerSpaceY = footerStartY + 35.0f;
        float sectionStep = gameUI.screenWidth / 3.0f;
        float pillW = 120.0f;
        float pillH = 64.0f;
        float pillY = innerSpaceY - 5.0f;

        if (gameUI.currentState == STATE_HOME) {
            float activePillX = (sectionStep * 0.5f) - (pillW / 2.0f);
            drawRoundRectNative(env, canvas, activePillX, pillY, activePillX + pillW, pillY + pillH, pillH/2.0f, pillH/2.0f, gameUI.isCurrentlyDark ? 0xFF2C354A : 0xFFE0E6FF);
        } else if (gameUI.currentState == STATE_LEVELS) {
            float activePillX = (sectionStep * 1.5f) - (pillW / 2.0f);
            drawRoundRectNative(env, canvas, activePillX, pillY, activePillX + pillW, pillY + pillH, pillH/2.0f, pillH/2.0f, gameUI.isCurrentlyDark ? 0xFF2C354A : 0xFFE0E6FF);
        } else if (gameUI.currentState == STATE_SETTINGS) {
            float activePillX = (sectionStep * 2.5f) - (pillW / 2.0f);
            drawRoundRectNative(env, canvas, activePillX, pillY, activePillX + pillW, pillY + pillH, pillH/2.0f, pillH/2.0f, gameUI.isCurrentlyDark ? 0xFF2C354A : 0xFFE0E6FF);
        }

        float homeX = (sectionStep * 0.5f) - (navIconSize / 2.0f);
        if (gameUI.assetBitmaps[ASSET_HOME]) {
            renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_HOME], homeX, innerSpaceY, navIconSize, (gameUI.currentState == STATE_HOME) ? tintActive : tintGray);
        }
        gameUI.UIButtons.push_back({0.0f, footerStartY, sectionStep, footerFixedBarHeight + 60.0f, 9001, 0});

        float lvlX = (sectionStep * 1.5f) - (navIconSize / 2.0f);
        if (gameUI.assetBitmaps[ASSET_LEVEL]) {
            renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_LEVEL], lvlX, innerSpaceY, navIconSize, (gameUI.currentState == STATE_LEVELS) ? tintActive : tintGray);
        }
        gameUI.UIButtons.push_back({sectionStep, footerStartY, sectionStep, footerFixedBarHeight + 60.0f, 9002, 0});

        float setX = (sectionStep * 2.5f) - (navIconSize / 2.0f);
        if (gameUI.assetBitmaps[ASSET_SETTINGS]) {
            renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_SETTINGS], setX, innerSpaceY, navIconSize, (gameUI.currentState == STATE_SETTINGS) ? tintActive : tintGray);
        }
        gameUI.UIButtons.push_back({sectionStep * 2.0f, footerStartY, sectionStep, footerFixedBarHeight + 60.0f, 9003, 0});
    }

    // --- 6. OVERLAY DIALOGS ---
    if (activeModalBlocks) {
        drawRoundRectNative(env, canvas, 0, 0, gameUI.screenWidth, gameUI.screenHeight, 0, 0, 0xAA000000);

        float dW = gameUI.screenWidth * 0.84f;
        float dH = gameUI.screenHeight * 0.24f; 
        
        if (gameUI.isHintPopupActive) {
            dH = gameUI.screenHeight * 0.25f; 
        } else if (gameUI.isPausePopupActive) {
            dH = gameUI.screenHeight * 0.22f;
        } else if (gameUI.isRatingPopupActive || localIsAdWatchPopupActive) {
            dH = gameUI.screenHeight * 0.35f;
        }

        float dX = (gameUI.screenWidth - dW) / 2.0f;
        float dY = (gameUI.screenHeight - dH) / 2.0f;

        jclass paintCls = gameUI.paintTextReference ? env->GetObjectClass(gameUI.paintTextReference) : nullptr;
        jmethodID setTextSize = paintCls ? env->GetMethodID(paintCls, "setTextSize", "(F)V") : nullptr;
        jmethodID setColor = paintCls ? env->GetMethodID(paintCls, "setColor", "(I)V");
        jmethodID measureText = paintCls ? env->GetMethodID(paintCls, "measureText", "(Ljava/lang/String;)F") : nullptr;

        if (!gameUI.isPausePopupActive) {
            drawRoundRectNative(env, canvas, dX, dY, dX + dW, dY + dH, 44, 44, gameUI.isCurrentlyDark ? COLOR_DARK_CARD : COLOR_LIGHT_CARD);
            
            float closeBtnSize = 44.0f;
            float closeX = dX + dW - closeBtnSize - 25.0f;
            float closeY = dY + 25.0f;
            if (gameUI.assetBitmaps[ASSET_CLOSE]) {
                renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_CLOSE], closeX, closeY, closeBtnSize, tintGray);
                gameUI.UIButtons.push_back({closeX - 20.0f, closeY - 20.0f, closeBtnSize + 40.0f, closeBtnSize + 40.0f, 9999, 0});
            }
        }

        // --- AD WATCH POPUP INTERLAY ---
        if (localIsAdWatchPopupActive) {
            if (paintCls && setTextSize && setColor && measureText) {
                setPaintFontWeight(env, gameUI.paintTextReference, true);
                env->CallVoidMethod(gameUI.paintTextReference, setTextSize, 40.0f);
                env->CallVoidMethod(gameUI.paintTextReference, setColor, baseTxtColor);
                
                jstring lockTitle = env->NewStringUTF("Unlock Level");
                jstring lockDesc = env->NewStringUTF("Watch a short ad to unlock");
                jstring lockDescSub = env->NewStringUTF("and play this level layout!");

                float tW1 = env->CallFloatMethod(gameUI.paintTextReference, measureText, lockTitle);
                env->CallVoidMethod(canvas, gameUI.midDrawText, lockTitle, dX + (dW - tW1) / 2.0f, dY + 90.0f, gameUI.paintTextReference);

                env->CallVoidMethod(gameUI.paintTextReference, setTextSize, 32.0f);
                float tW2 = env->CallFloatMethod(gameUI.paintTextReference, measureText, lockDesc);
                float tW3 = env->CallFloatMethod(gameUI.paintTextReference, measureText, lockDescSub);
                
                env->CallVoidMethod(canvas, gameUI.midDrawText, lockDesc, dX + (dW - tW2) / 2.0f, dY + 145.0f, gameUI.paintTextReference);
                env->CallVoidMethod(canvas, gameUI.midDrawText, lockDescSub, dX + (dW - tW3) / 2.0f, dY + 190.0f, gameUI.paintTextReference);

                env->DeleteLocalRef(lockTitle);
                env->DeleteLocalRef(lockDesc);
                env->DeleteLocalRef(lockDescSub);
                setPaintFontWeight(env, gameUI.paintTextReference, false);
            }

            float actBtnW = dW * 0.75f;
            float actBtnH = 85.0f;
            float actX = dX + (dW - actBtnW) / 2.0f;
            float actY = dY + dH - actBtnH - 35.0f;
            
            drawDialogButton(env, canvas, actX, actY, actBtnW, actBtnH, "WATCH AD TO PLAY", 0xFFE58E26, 0xFFFFFFFF);
            gameUI.UIButtons.push_back({actX, actY, actBtnW, actBtnH, 7850, 0}); 
        }

        // --- RATE MY APP POPUP INTERLAY ---
        if (gameUI.isRatingPopupActive) { 
            if (paintCls && setTextSize && setColor && measureText) {
                setPaintFontWeight(env, gameUI.paintTextReference, true);
                env->CallVoidMethod(gameUI.paintTextReference, setTextSize, 40.0f);
                env->CallVoidMethod(gameUI.paintTextReference, setColor, baseTxtColor);
                
                jstring rateHeader = env->NewStringUTF("Rate my app on Play Store");
                float twTitle = env->CallFloatMethod(gameUI.paintTextReference, measureText, rateHeader);
                env->CallVoidMethod(canvas, gameUI.midDrawText, rateHeader, dX + (dW - twTitle) / 2.0f, dY + 95.0f, gameUI.paintTextReference);
                env->DeleteLocalRef(rateHeader);
                setPaintFontWeight(env, gameUI.paintTextReference, false);
            }

            float starSize = 60.0f;
            float totalStarsWidth = (5 * starSize) + (4 * 20.0f);
            float startStarX = dX + (dW - totalStarsWidth) / 2.0f;
            float starY = dY + 150.0f;

            if (gameUI.assetBitmaps[ASSET_STAR]) { 
                for (int s = 0; s < 5; s++) {
                    float currentStarX = startStarX + s * (starSize + 20.0f);
                    jobject starColorTint = (s < nativeRatingScore) ? tintYellow : tintWhite;
                    renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_STAR], currentStarX, starY, starSize, starColorTint);
                    gameUI.UIButtons.push_back({currentStarX, starY, starSize, starSize, 8001 + s, 0});
                }
            }

            float actionBtnW = dW * 0.65f;
            float actionBtnH = 85.0f;
            float actionX = dX + (dW - actionBtnW) / 2.0f;
            float actionY = dY + dH - actionBtnH - 35.0f;
            
            drawDialogButton(env, canvas, actionX, actionY, actionBtnW, actionBtnH, "SUBMIT", COLOR_ACCENT_BLUE, 0xFFFFFFFF);
            gameUI.UIButtons.push_back({actionX, actionY, actionBtnW, actionBtnH, 8500, 0}); 
        }

        // --- HINT POPUP INTERLAY ---
        if (gameUI.isHintPopupActive) {
            if (paintCls && setTextSize && setColor && measureText) {
                setPaintFontWeight(env, gameUI.paintTextReference, true);
                env->CallVoidMethod(gameUI.paintTextReference, setTextSize, 36.0f);
                env->CallVoidMethod(gameUI.paintTextReference, setColor, baseTxtColor);
                
                jstring hintHeader = env->NewStringUTF("Need Help?");
                jstring hintSubText = env->NewStringUTF("Watch ads to get a hint!");
                
                float hW1 = env->CallFloatMethod(gameUI.paintTextReference, measureText, hintHeader);
                float hW2 = env->CallFloatMethod(gameUI.paintTextReference, measureText, hintSubText);
                
                env->CallVoidMethod(canvas, gameUI.midDrawText, hintHeader, dX + (dW - hW1)/2.0f, dY + 75.0f, gameUI.paintTextReference);
                env->CallVoidMethod(canvas, gameUI.midDrawText, hintSubText, dX + (dW - hW2)/2.0f, dY + 120.0f, gameUI.paintTextReference);
                
                env->DeleteLocalRef(hintHeader);
                env->DeleteLocalRef(hintSubText);
                setPaintFontWeight(env, gameUI.paintTextReference, false);
            }

            float actBtnW = dW * 0.70f;
            float actBtnH = 80.0f;
            float actX = dX + (dW - actBtnW) / 2.0f;
            float actY = dY + dH - actBtnH - 25.0f; 
            
            drawDialogButton(env, canvas, actX, actY, actBtnW, actBtnH, "WATCH ADS", COLOR_ACCENT_BLUE, 0xFFFFFFFF);
            gameUI.UIButtons.push_back({actX, actY, actBtnW, actBtnH, 4003, 0});
        }

        if (gameUI.isPausePopupActive) {
            drawHorizontalPausePopup(env, canvas, dX, dY, dW, dH, tintActive);
        }
    }
}

JNIEXPORT jboolean JNICALL
Java_com_night_backgroundchange_MainActivity_handleNativeTouch(JNIEnv* env, jobject obj, jfloat tx, jfloat ty) {
    // Process input coordinates from the top layer downwards
    for (int i = (int)gameUI.UIButtons.size() - 1; i >= 0; i--) {
        auto& btn = gameUI.UIButtons[i];
        if (tx >= btn.x && tx <= (btn.x + btn.w) && ty >= btn.y && ty <= (btn.y + btn.h)) {
            
            // Global modal dismissing identifier block
            if (btn.action == 9999) {
                gameUI.isHintPopupActive = false;
                gameUI.isThemePopupActive = false;
                gameUI.isRatingPopupActive = false;
                localIsAdWatchPopupActive = false;
                return JNI_TRUE;
            }
            
            // Interactive Rating Component
            if (btn.action >= 8001 && btn.action <= 8005) {
                nativeRatingScore = (btn.action - 8000);
                return JNI_TRUE;
            }
            if (btn.action == 8500) { 
                gameUI.isRatingPopupActive = false;
                return JNI_TRUE;
            }

            // Bottom Navigation View Redirection Routers
            if (btn.action == 9001) { gameUI.currentState = STATE_HOME; return JNI_TRUE; }
            if (btn.action == 9002) { gameUI.currentState = STATE_LEVELS; return JNI_TRUE; }
            if (btn.action == 9003) { gameUI.currentState = STATE_SETTINGS; return JNI_TRUE; }

            // Gameplay Interface Toggles
            if (btn.action == 4002) { gameUI.isPausePopupActive = true; return JNI_TRUE; }
            if (btn.action == 4003) { gameUI.isHintPopupActive = !gameUI.isHintPopupActive; return JNI_TRUE; }
            
            // Pause Popups Handlers
            if (btn.action == 5501) { gameUI.isPausePopupActive = false; return JNI_TRUE; }
            if (btn.action == 5502) { gameUI.isPausePopupActive = false; return JNI_TRUE; }
            if (btn.action == 5504) { gameUI.isPausePopupActive = false; gameUI.currentState = STATE_HOME; return JNI_TRUE; }

            // Triggering setting configurations
            if (btn.action == 6501) { gameUI.isCurrentlyDark = !gameUI.isCurrentlyDark; return JNI_TRUE; }
            if (btn.action == 6504) { gameUI.isRatingPopupActive = true; nativeRatingScore = 0; return JNI_TRUE; }

            // Orange Ad Level Processing Hook
            if (btn.action >= 7800 && btn.action < 7850) {
                pendingUnlockLevelIndex = btn.param;
                localIsAdWatchPopupActive = true;
                return JNI_TRUE;
            }
            
            // Action processing from Ad Watch Popup target click
            if (btn.action == 7850) {
                if (pendingUnlockLevelIndex >= 0 && pendingUnlockLevelIndex < 50) {
                    gameUI.levelsUnlocked[pendingUnlockLevelIndex] = true;
                    gameUI.currentPlayingLevel = pendingUnlockLevelIndex + 1;
                    gameUI.currentState = STATE_GAMEPLAY;
                }
                localIsAdWatchPopupActive = false;
                pendingUnlockLevelIndex = -1;
                return JNI_TRUE;
            }

            // Normal unlocked level transition router
            if (btn.action >= 3000 && btn.action < 3050) {
                gameUI.currentPlayingLevel = btn.param + 1;
                gameUI.currentState = STATE_GAMEPLAY;
                return JNI_TRUE;
            }
            
            if (btn.action == 2001) {
                gameUI.currentState = STATE_GAMEPLAY;
                return JNI_TRUE;
            }
        }
    }
    return JNI_FALSE;
}

} // extern "C"
