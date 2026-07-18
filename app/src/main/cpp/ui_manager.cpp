#include "game_structures.h"
#include <cmath>
#include <string>

// Premium Modern UI Palette Constants
#define COLOR_DARK_BG       0xFF161A23  // Premium deep slate navy
#define COLOR_LIGHT_BG      0xFFF5F7FA  // Clean crisp off-white
#define COLOR_DARK_CARD     0xFF222A3A  // Deep card contrast background
#define COLOR_LIGHT_CARD    0xFFFFFFFF  // Pure white card contrast
#define COLOR_ACCENT_BLUE   0xFF5773FF  // Vibrant primary blue/violet branding
#define COLOR_TEXT_MUTED    0xFF7E8B9B  // Sleek secondary placeholder color

static int nativeRatingScore = 4; 
static float gameplayZoomScale = 1.0f; 
static bool localIsAdWatchPopupActive = false; 

void checkGlobalClosePopupDismiss(float touchX, float touchY) {
    // Dismiss mechanics managed cleanly inside your touch coordinator pipeline
}

// Upgraded Helper to draw beautiful modern pill/capsule buttons
void drawDialogButton(JNIEnv* env, jobject canvas, float x, float y, float w, float h, const char* label, int bgColor, int textColor) {
    // Render perfect smooth capsule styling with dynamic height-based corner radius
    float cornerRadius = h / 2.0f;
    drawRoundRectNative(env, canvas, x, y, x + w, y + h, cornerRadius, cornerRadius, bgColor);
    
    if (gameUI.paintTextReference) {
        setPaintFontWeight(env, gameUI.paintTextReference, true);
        jclass paintCls = env->GetObjectClass(gameUI.paintTextReference);
        jmethodID setTextSize = env->GetMethodID(paintCls, "setTextSize", "(F)V");
        jmethodID setColor = env->GetMethodID(paintCls, "setColor", "(I)V");
        
        env->CallVoidMethod(gameUI.paintTextReference, setTextSize, 34.0f);
        env->CallVoidMethod(gameUI.paintTextReference, setColor, textColor);
        
        jstring jText = env->NewStringUTF(label);
        jmethodID measureText = env->GetMethodID(paintCls, "measureText", "(Ljava/lang/String;)F");
        float textW = env->CallFloatMethod(gameUI.paintTextReference, measureText, jText);
        
        float tx = x + (w - textW) / 2.0f;
        float ty = y + (h / 2.0f) + 11.0f; 
        env->CallVoidMethod(canvas, gameUI.midDrawText, jText, tx, ty, gameUI.paintTextReference);
        env->DeleteLocalRef(jText);
        setPaintFontWeight(env, gameUI.paintTextReference, false);
    }
}

// Render clean minimalist branding text alignment matching reference title cards
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
    float fixedHeaderY = gameUI.screenHeight * 0.52f; // Centered beautifully relative to branding viewports

    jstring runStr = env->NewStringUTF("A");
    jstring rrowStr = env->NewStringUTF("rrows");

    float runWidth = env->CallFloatMethod(gameUI.paintTextReference, measureText, runStr);
    float rrowWidth = env->CallFloatMethod(gameUI.paintTextReference, measureText, rrowStr);
    
    float inlineArrowSize = 55.0f;
    float totalHeaderWidth = inlineArrowSize + rrowWidth + 4.0f;
    float startX = midPointX - (totalHeaderWidth / 2.0f);

    float arrowY = fixedHeaderY - 54.0f;
    if (gameUI.assetBitmaps[ASSET_ARROW]) {
        renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_ARROW], startX, arrowY, inlineArrowSize, tintRed);
    }

    float rrowX = startX + inlineArrowSize + 4.0f;
    env->CallVoidMethod(canvas, gameUI.midDrawText, rrowStr, rrowX, fixedHeaderY, gameUI.paintTextReference);

    env->DeleteLocalRef(runStr);
    env->DeleteLocalRef(rrowStr);
    setPaintFontWeight(env, gameUI.paintTextReference, false);
}

void drawHorizontalPausePopup(JNIEnv* env, jobject canvas, float dX, float dY, float dW, float dH, jobject tintActive) {
    float forcedSquareDim = dW > dH ? dH : dW;
    float squareLeft = dX + (dW - forcedSquareDim) / 2.0f;
    float squareTop = dY + (dH - forcedSquareDim) / 2.0f;

    drawRoundRectNative(env, canvas, squareLeft, squareTop, squareLeft + forcedSquareDim, squareTop + forcedSquareDim, 44, 44, gameUI.isCurrentlyDark ? COLOR_DARK_CARD : COLOR_LIGHT_CARD);

    float buttonSize = forcedSquareDim * 0.18f; 
    float innerSpacingY = squareTop + (forcedSquareDim / 2.0f) - (buttonSize / 2.0f);
    float itemHorizontalStep = forcedSquareDim / 4.0f;

    float btn1X = squareLeft + itemHorizontalStep - (buttonSize / 2.0f);
    if (gameUI.assetBitmaps[ASSET_PLAY]) {
        renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_PLAY], btn1X, innerSpacingY, buttonSize, tintActive);
    }
    gameUI.UIButtons.push_back({btn1X, innerSpacingY, buttonSize, buttonSize, 5501, 0});

    float btn2X = squareLeft + (itemHorizontalStep * 2.0f) - (buttonSize / 2.0f);
    if (gameUI.assetBitmaps[ASSET_RETRY]) {
        renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_RETRY], btn2X, innerSpacingY, buttonSize, tintActive);
    }
    gameUI.UIButtons.push_back({btn2X, innerSpacingY, buttonSize, buttonSize, 5502, 0});

    float btn3X = squareLeft + (itemHorizontalStep * 3.0f) - (buttonSize / 2.0f);
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

    float headerFixedBarHeight = 160.0f;
    float footerFixedBarHeight = 160.0f;
    float footerStartY = gameUI.screenHeight - footerFixedBarHeight - 60.0f;

    // --- 0. LOADING SCREEN ---
    if (gameUI.currentState == STATE_LOADING) {
        if (gameUI.paintTextReference) {
            setPaintFontWeight(env, gameUI.paintTextReference, true);
            jclass paintCls = env->GetObjectClass(gameUI.paintTextReference);
            jmethodID setTextSize = env->GetMethodID(paintCls, "setTextSize", "(F)V");
            jmethodID setColor = env->GetMethodID(paintCls, "setColor", "(I)V");
            jmethodID measureText = env->GetMethodID(paintCls, "measureText", "(Ljava/lang/String;)F");

            env->CallVoidMethod(gameUI.paintTextReference, setTextSize, 45.0f);
            env->CallVoidMethod(gameUI.paintTextReference, setColor, unselectedGrayColor);

            jstring loadStr = env->NewStringUTF("Loading...");
            float strW = env->CallFloatMethod(gameUI.paintTextReference, measureText, loadStr);
            
            float lx = (gameUI.screenWidth - strW) / 2.0f;
            float ly = gameUI.screenHeight / 2.0f;
            env->CallVoidMethod(canvas, gameUI.midDrawText, loadStr, lx, ly, gameUI.paintTextReference);
            env->DeleteLocalRef(loadStr);
            setPaintFontWeight(env, gameUI.paintTextReference, false);
        }
        return;
    }

    // --- 1. HOME SCREEN ---
    if (gameUI.currentState == STATE_HOME) {
        // Top System Feature Card Layouts (Leagues / Challenge Lookalike Row)
        float cardW = (gameUI.screenWidth - 100.0f) / 2.0f;
        float cardH = 260.0f;
        float cardY = 120.0f;

        // Card 1: Leagues stub bounds
        drawRoundRectNative(env, canvas, 40.0f, cardY, 40.0f + cardW, cardY + cardH, 32, 32, baseCardColor);
        // Card 2: Challenge stub bounds
        drawRoundRectNative(env, canvas, gameUI.screenWidth - 40.0f - cardW, cardY, gameUI.screenWidth - 40.0f, cardY + cardH, 32, 32, baseCardColor);

        // Core App Title branding render
        drawGameHeader(env, obj, canvas, baseBgColor, baseTxtColor, tintWhite);

        // Under-title Level Label Info Text
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
            env->CallVoidMethod(canvas, gameUI.midDrawText, jLvlStr, (gameUI.screenWidth - lvlW) / 2.0f, (gameUI.screenHeight * 0.52f) + 65.0f, gameUI.paintTextReference);
            env->DeleteLocalRef(jLvlStr);
            setPaintFontWeight(env, gameUI.paintTextReference, false);
        }

        // Modern Clean Wide Primary Play Capsule Button
        float playW = gameUI.screenWidth * 0.68f;
        float playH = 120.0f;
        float playX = (gameUI.screenWidth - playW) / 2.0f;
        float playY = footerStartY - 100.0f;

        drawDialogButton(env, canvas, playX, playY, playW, playH, "Play", COLOR_ACCENT_BLUE, 0xFFFFFFFF);
        gameUI.UIButtons.push_back({playX, playY, playW, playH, 2001, 0});
    }

    // --- 2. SELECT LEVEL SCREEN ---
    if (gameUI.currentState == STATE_LEVELS) {
        float boxSize = gameUI.screenWidth * 0.25f;
        float spaceGrid = gameUI.screenWidth * 0.04f;
        float offsetGridX = (gameUI.screenWidth - (3 * boxSize + 2 * spaceGrid)) / 2.0f;
        
        int totalRows = (int)std::ceil(50.0f / 3.0f);
        float extensionButtonHeight = 90.0f;
        float totalContentHeight = totalRows * (boxSize + spaceGrid) + extensionButtonHeight + 120.0f;
        gameUI.maxScrollExtent = totalContentHeight - (footerStartY - headerFixedBarHeight);
        if (gameUI.maxScrollExtent < 0.0f) gameUI.maxScrollExtent = 0.0f;

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
            
            if (gameUI.levelsUnlocked[i]) {
                if (i == gameUI.currentPlayingLevel - 1 || i == 0) {
                    finalBoxColor = COLOR_ACCENT_BLUE;
                    textLvlColor = 0xFFFFFFFF;
                }
            } else {
                if (i > 0 && gameUI.levelsUnlocked[i - 1]) {
                    finalBoxColor = 0xFFE58E26; // Highlighted Locked Ad-State Row
                }
            }
            
            drawRoundRectNative(env, canvas, bx, by, bx + boxSize, by + boxSize, 24, 24, finalBoxColor);

            if (!gameUI.levelsUnlocked[i]) {
                float iconSz = boxSize * 0.35f;
                float iconOffset = (boxSize - iconSz) / 2.0f;
                if (i > 0 && gameUI.levelsUnlocked[i - 1]) {
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
            } else if (i > 0 && gameUI.levelsUnlocked[i - 1]) {
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

        // Fixed Top Header Panel
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
        float centeredHeaderX = (gameUI.screenWidth / 2.0f) - (headerTextW / 2.0f);
        
        env->CallVoidMethod(canvas, gameUI.midDrawText, levelHeader, centeredHeaderX, 105.0f, gameUI.paintTextReference);
        env->DeleteLocalRef(levelHeader);
        setPaintFontWeight(env, gameUI.paintTextReference, false);
    }

    // --- 3. SETTINGS MENU VIEW (Professional Grouped Card Layout) ---
    if (gameUI.currentState == STATE_SETTINGS) {
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

            // Draw sleek modern switch element container stub for option index 0
            if (i == 0) {
                float swW = 85.0f;
                float swH = 46.0f;
                float swX = marginX + rowWidth - swW - 40.0f;
                float swY = optionY + (optionHeight - swH) / 2.0f;
                drawRoundRectNative(env, canvas, swX, swY, swX + swW, swY + swH, swH/2.0f, swH/2.0f, gameUI.isCurrentlyDark ? COLOR_ACCENT_BLUE : unselectedGrayColor);
                drawRoundRectNative(env, canvas, gameUI.isCurrentlyDark ? (swX + swW - 40.0f) : (swX + 6.0f), swY + 5.0f, gameUI.isCurrentlyDark ? (swX + swW - 6.0f) : (swX + swH - 11.0f), swY + swH - 5.0f, 18, 18, 0xFFFFFFFF);
            }

            gameUI.UIButtons.push_back({marginX, optionY, rowWidth, optionHeight, optionActions[i], 0});
            optionY += optionHeight + optionSpacing;
        }
    }

    // --- 4. GAMEPLAY PLAYGROUND SCREEN ---
    if (gameUI.currentState == STATE_GAMEPLAY) {
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
            float bannerX = (gameUI.screenWidth / 2.0f) - (textW / 2.0f);

            env->CallVoidMethod(canvas, gameUI.midDrawText, jBanner, bannerX, 95.0f, gameUI.paintTextReference);
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

        env->CallVoidMethod(canvas, midDrawLine, nodeStartX, nodeStartY, nodeEndX, nodeStartY, linePaint);
        env->CallVoidMethod(canvas, midDrawLine, nodeEndX, nodeStartY, nodeEndX, nodeEndY, linePaint);

        if (gameUI.assetBitmaps[ASSET_ARROW]) {
            env->CallIntMethod(canvas, gameUI.midSave);
            jmethodID midRotate = env->GetMethodID(env->GetObjectClass(canvas), "rotate", "(FFF)V");
            env->CallVoidMethod(canvas, midRotate, 180.0f, nodeEndX, nodeEndY);
            renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_ARROW], nodeEndX - 25.0f, nodeEndY - 25.0f, 50.0f, tintWhite);
            env->CallVoidMethod(canvas, midRotate, 0.0f, nodeStartX, nodeStartY); 
            env->CallVoidMethod(canvas, gameUI.midRestore);
        }

        env->CallVoidMethod(canvas, gameUI.midRestore); 

        env->DeleteLocalRef(dotPaint);
        env->DeleteLocalRef(linePaint);
        env->DeleteLocalRef(paintClass);
    }

    // --- 5. IMMERSIVE BOTTOM NAVIGATION VIEW BAR ---
    if (gameUI.currentState == STATE_HOME || gameUI.currentState == STATE_SETTINGS || gameUI.currentState == STATE_LEVELS) {
        drawRoundRectNative(env, canvas, 0, footerStartY, gameUI.screenWidth, gameUI.screenHeight, 0, 0, baseCardColor);

        float navIconSize = 55.0f; 
        float innerSpaceY = footerStartY + 35.0f;
        float sectionStep = gameUI.screenWidth / 3.0f;

        // Custom Capsule Pill Indicator Highlight background matching modern specs
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

        if (gameUI.assetBitmaps[ASSET_HOME]) {
            jobject homePaint = (gameUI.currentState == STATE_HOME) ? tintActive : tintGray;
            float homeX = (sectionStep * 0.5f) - (navIconSize / 2.0f);
            renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_HOME], homeX, innerSpaceY, navIconSize, homePaint);
            gameUI.UIButtons.push_back({homeX - 20.0f, footerStartY, navIconSize + 40.0f, footerFixedBarHeight, 9001, 0});
        }

        if (gameUI.assetBitmaps[ASSET_LEVEL]) {
            jobject lvlPaint = (gameUI.currentState == STATE_LEVELS) ? tintActive : tintGray;
            float lvlX = (sectionStep * 1.5f) - (navIconSize / 2.0f);
            renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_LEVEL], lvlX, innerSpaceY, navIconSize, lvlPaint);
            gameUI.UIButtons.push_back({lvlX - 20.0f, footerStartY, navIconSize + 40.0f, footerFixedBarHeight, 9002, 0});
        }

        if (gameUI.assetBitmaps[ASSET_SETTINGS]) {
            jobject setPaint = (gameUI.currentState == STATE_SETTINGS) ? tintActive : tintGray;
            float setX = (sectionStep * 2.5f) - (navIconSize / 2.0f);
            renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_SETTINGS], setX, innerSpaceY, navIconSize, setPaint);
            gameUI.UIButtons.push_back({setX - 20.0f, footerStartY, navIconSize + 40.0f, footerFixedBarHeight, 9003, 0});
        }
    }

    // --- 6. GLOBAL DIALOG POPUPS & INTERFACE INTERLAYS ---
    bool activeModalBlocks = (gameUI.isHintPopupActive || gameUI.isThemePopupActive || gameUI.isRatingPopupActive || gameUI.isPausePopupActive || localIsAdWatchPopupActive);

    if (activeModalBlocks) {
        drawRoundRectNative(env, canvas, 0, 0, gameUI.screenWidth, gameUI.screenHeight, 0, 0, 0xAA000000);

        float dW = gameUI.screenWidth * 0.84f;
        float dH = gameUI.screenHeight * 0.42f; 
        float dX = (gameUI.screenWidth - dW) / 2.0f;
        float dY = (gameUI.screenHeight - dH) / 2.0f;

        drawRoundRectNative(env, canvas, dX, dY, dX + dW, dY + dH, 44, 44, gameUI.isCurrentlyDark ? COLOR_DARK_CARD : COLOR_LIGHT_CARD);

        jclass paintCls = gameUI.paintTextReference ? env->GetObjectClass(gameUI.paintTextReference) : nullptr;
        jmethodID setTextSize = paintCls ? env->GetMethodID(paintCls, "setTextSize", "(F)V") : nullptr;
        jmethodID setColor = paintCls ? env->GetMethodID(paintCls, "setColor", "(I)V") : nullptr;
        jmethodID measureText = paintCls ? env->GetMethodID(paintCls, "measureText", "(Ljava/lang/String;)F") : nullptr;

        float closeBtnSize = 48.0f;
        float closeX = dX + dW - closeBtnSize - 30.0f;
        float closeY = dY + 30.0f;
        if (gameUI.assetBitmaps[ASSET_CLOSE]) {
            renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_CLOSE], closeX, closeY, closeBtnSize, tintGray);
            gameUI.UIButtons.push_back({closeX - 15.0f, closeY - 15.0f, closeBtnSize + 30.0f, closeBtnSize + 30.0f, 9999, 0});
        }

        // --- AD UNLOCK FEATURES MODAL INTERLAY ---
        if (localIsAdWatchPopupActive) {
            if (paintCls && setTextSize && setColor && measureText) {
                setPaintFontWeight(env, gameUI.paintTextReference, true);
                env->CallVoidMethod(gameUI.paintTextReference, setTextSize, 42.0f);
                env->CallVoidMethod(gameUI.paintTextReference, setColor, baseTxtColor);
                
                jstring lockTitle = env->NewStringUTF("Level is Locked!");
                jstring lockDesc = env->NewStringUTF("Tap below to watch ads and");
                jstring lockDescSub = env->NewStringUTF("unlock this level instantly!");

                float tW1 = env->CallFloatMethod(gameUI.paintTextReference, measureText, lockTitle);
                env->CallVoidMethod(canvas, gameUI.midDrawText, lockTitle, dX + (dW - tW1) / 2.0f, dY + 110.0f, gameUI.paintTextReference);

                env->CallVoidMethod(gameUI.paintTextReference, setTextSize, 34.0f);
                float tW2 = env->CallFloatMethod(gameUI.paintTextReference, measureText, lockDesc);
                float tW3 = env->CallFloatMethod(gameUI.paintTextReference, measureText, lockDescSub);
                
                env->CallVoidMethod(canvas, gameUI.midDrawText, lockDesc, dX + (dW - tW2) / 2.0f, dY + 175.0f, gameUI.paintTextReference);
                env->CallVoidMethod(canvas, gameUI.midDrawText, lockDescSub, dX + (dW - tW3) / 2.0f, dY + 225.0f, gameUI.paintTextReference);

                env->DeleteLocalRef(lockTitle);
                env->DeleteLocalRef(lockDesc);
                env->DeleteLocalRef(lockDescSub);
                setPaintFontWeight(env, gameUI.paintTextReference, false);
            }

            float actBtnW = dW * 0.75f;
            float actBtnH = 90.0f;
            float actX = dX + (dW - actBtnW) / 2.0f;
            float actY = dY + dH - actBtnH - 45.0f;
            
            drawDialogButton(env, canvas, actX, actY, actBtnW, actBtnH, "WATCH AD TO PLAY", 0xFFE58E26, 0xFFFFFFFF);
            gameUI.UIButtons.push_back({actX, actY, actBtnW, actBtnH, 7850, 0}); 
        }

        if (gameUI.isRatingPopupActive) { 
            if (paintCls && setTextSize && setColor && measureText) {
                setPaintFontWeight(env, gameUI.paintTextReference, true);
                env->CallVoidMethod(gameUI.paintTextReference, setTextSize, 42.0f);
                env->CallVoidMethod(gameUI.paintTextReference, setColor, baseTxtColor);
                
                jstring rateHeader = env->NewStringUTF("Rate my app on Play Store");
                float twTitle = env->CallFloatMethod(gameUI.paintTextReference, measureText, rateHeader);
                env->CallVoidMethod(canvas, gameUI.midDrawText, rateHeader, dX + (dW - twTitle) / 2.0f, dY + 110.0f, gameUI.paintTextReference);
                env->DeleteLocalRef(rateHeader);
                setPaintFontWeight(env, gameUI.paintTextReference, false);
            }

            float starSize = 65.0f;
            float totalStarsWidth = (5 * starSize) + (4 * 20.0f);
            float startStarX = dX + (dW - totalStarsWidth) / 2.0f;
            float starY = dY + 170.0f;

            if (gameUI.assetBitmaps[ASSET_STAR]) { 
                for (int s = 0; s < 5; s++) {
                    float currentStarX = startStarX + s * (starSize + 20.0f);
                    jobject starColorTint = (s < nativeRatingScore) ? tintYellow : tintGray;
                    renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_STAR], currentStarX, starY, starSize, starColorTint);
                    gameUI.UIButtons.push_back({currentStarX, starY, starSize, starSize, 8001 + s, 0});
                }
            }

            float actionBtnW = dW * 0.65f;
            float actionBtnH = 85.0f;
            float actionX = dX + (dW - actionBtnW) / 2.0f;
            float actionY = dY + dH - actionBtnH - 40.0f;
            
            drawDialogButton(env, canvas, actionX, actionY, actionBtnW, actionBtnH, "SUBMIT", COLOR_ACCENT_BLUE, 0xFFFFFFFF);
            gameUI.UIButtons.push_back({actionX, actionY, actionBtnW, actionBtnH, 8500, 0}); 
        }

        if (gameUI.isHintPopupActive) {
            if (paintCls && setTextSize && setColor && measureText) {
                setPaintFontWeight(env, gameUI.paintTextReference, true);
                env->CallVoidMethod(gameUI.paintTextReference, setTextSize, 38.0f);
                env->CallVoidMethod(gameUI.paintTextReference, setColor, baseTxtColor);
                
                jstring hintHeader = env->NewStringUTF("Need Help?");
                jstring hintSubText = env->NewStringUTF("Watch ads to get a hint!");
                
                float hW1 = env->CallFloatMethod(gameUI.paintTextReference, measureText, hintHeader);
                float hW2 = env->CallFloatMethod(gameUI.paintTextReference, measureText, hintSubText);
                
                env->CallVoidMethod(canvas, gameUI.midDrawText, hintHeader, dX + (dW - hW1)/2.0f, dY + 95.0f, gameUI.paintTextReference);
                env->CallVoidMethod(canvas, gameUI.midDrawText, hintSubText, dX + (dW - hW2)/2.0f, dY + 145.0f, gameUI.paintTextReference);
                
                env->DeleteLocalRef(hintHeader);
                env->DeleteLocalRef(hintSubText);
                setPaintFontWeight(env, gameUI.paintTextReference, false);
            }

            float actBtnW = dW * 0.70f;
            float actBtnH = 85.0f;
            float actX = dX + (dW - actBtnW) / 2.0f;
            float actY = dY + dH - actBtnH - 35.0f;
            
            drawDialogButton(env, canvas, actX, actY, actBtnW, actBtnH, "WATCH ADS", COLOR_ACCENT_BLUE, 0xFFFFFFFF);
            gameUI.UIButtons.push_back({actX, actY, actBtnW, actBtnH, 4003, 0});
        }

        if (gameUI.isThemePopupActive) {
            if (paintCls && setTextSize && setColor && measureText) {
                setPaintFontWeight(env, gameUI.paintTextReference, true);
                env->CallVoidMethod(gameUI.paintTextReference, setTextSize, 38.0f);
                env->CallVoidMethod(gameUI.paintTextReference, setColor, baseTxtColor);

                jstring themeHeader = env->NewStringUTF("Choose Theme");
                float thW = env->CallFloatMethod(gameUI.paintTextReference, measureText, themeHeader);
                env->CallVoidMethod(canvas, gameUI.midDrawText, themeHeader, dX + (dW - thW)/2.0f, dY + 80.0f, gameUI.paintTextReference);
                env->DeleteLocalRef(themeHeader);
                setPaintFontWeight(env, gameUI.paintTextReference, false);
            }

            float colW = (dW - 70.0f) / 2.0f;
            float colH = 90.0f;
            float rowY = dY + dH - colH - 45.0f;

            float btnLightX = dX + 30.0f;
            drawDialogButton(env, canvas, btnLightX, rowY, colW, colH, "LIGHT", 0xFFE9ECEF, 0xFF000000);
            gameUI.UIButtons.push_back({btnLightX, rowY, colW, colH, 7701, 0});

            float btnDarkX = dX + 40.0f + colW;
            drawDialogButton(env, canvas, btnDarkX, rowY, colW, colH, "DARK", COLOR_DARK_BG, 0xFFFFFFFF);
            gameUI.UIButtons.push_back({btnDarkX, rowY, colW, colH, 7700, 0});
        }

        if (gameUI.isPausePopupActive) {
            drawHorizontalPausePopup(env, canvas, dX, dY, dW, gameUI.screenHeight * 0.44f, tintActive);
        }
    }
}

JNIEXPORT void JNICALL
Java_com_night_backgroundchange_MainActivity_setGameplayZoomFactor(JNIEnv* env, jobject obj, jfloat scaleFactor) {
    if (scaleFactor >= 0.5f && scaleFactor <= 2.5f) {
        gameplayZoomScale = scaleFactor;
    }
}

JNIEXPORT void JNICALL
Java_com_night_backgroundchange_MainActivity_setAdWatchPopupState(JNIEnv* env, jobject obj, jboolean isOpen) {
    localIsAdWatchPopupActive = (bool)isOpen;
}

} // extern "C"
