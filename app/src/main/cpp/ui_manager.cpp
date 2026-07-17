#include "game_structures.h"

void drawGameHeader(JNIEnv* env, jobject obj, jobject canvas, int baseBgColor, int baseTxtColor, jobject tintRed) {
    if (!gameUI.paintTextReference) return;
    
    setPaintFontWeight(env, gameUI.paintTextReference, true);
    jclass paintCls = env->GetObjectClass(gameUI.paintTextReference);
    jmethodID setTextSize = env->GetMethodID(paintCls, "setTextSize", "(F)V");
    jmethodID setColor = env->GetMethodID(paintCls, "setColor", "(I)V");
    
    // Make text scale cleanly down (From 85.0f to 65.0f)
    env->CallVoidMethod(gameUI.paintTextReference, setTextSize, 65.0f);
    env->CallVoidMethod(gameUI.paintTextReference, setColor, baseTxtColor);

    float midPointX = gameUI.screenWidth / 2.0f;
    float fixedHeaderY = 130.0f;

    // Center "RUN ARROW" as a single unified text string cleanly
    jstring titleText = env->NewStringUTF("RUN ARROW");
    jmethodID measureText = env->GetMethodID(paintCls, "measureText", "(Ljava/lang/String;)F");
    float textWidth = env->CallFloatMethod(gameUI.paintTextReference, measureText, titleText);
    float startTextX = midPointX - (textWidth / 2.0f);

    env->CallVoidMethod(canvas, gameUI.midDrawText, titleText, startTextX, fixedHeaderY, gameUI.paintTextReference);
    env->DeleteLocalRef(titleText);

    // Place the indicator arrow cleanly right before the letter "R" in "RUN"
    float headerArrowSize = 55.0f; // Scale down from 85.0f
    float headerArrowX = startTextX - headerArrowSize - 15.0f;
    float headerArrowY = fixedHeaderY - 50.0f;

    if (gameUI.assetBitmaps[ASSET_ARROW]) {
        jclass canvasCls = env->GetObjectClass(canvas);
        jmethodID midRotate = env->GetMethodID(canvasCls, "rotate", "(FFF)V");
        
        env->CallIntMethod(canvas, gameUI.midSave);
        if (midRotate) env->CallVoidMethod(canvas, midRotate, 270.0f, headerArrowX + (headerArrowSize / 2.0f), headerArrowY + (headerArrowSize / 2.0f));
        renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_ARROW], headerArrowX, headerArrowY, headerArrowSize, tintRed);
        env->CallVoidMethod(canvas, gameUI.midRestore);
    }
}

void drawWatermark(JNIEnv* env, jobject canvas) {
    // Watermark removed to keep your screen entirely clean!
}

void drawHorizontalPausePopup(JNIEnv* env, jobject canvas, float dX, float dY, float dW, float dH, jobject tintActive) {
    float forcedSquareDim = dW > dH ? dH : dW;
    float squareLeft = dX + (dW - forcedSquareDim) / 2.0f;
    float squareTop = dY + (dH - forcedSquareDim) / 2.0f;

    drawRoundRectNative(env, canvas, squareLeft, squareTop, squareLeft + forcedSquareDim, squareTop + forcedSquareDim, 36, 36, gameUI.isCurrentlyDark ? 0xFF1E1E1E : 0xFFFFFFFF);

    float buttonSize = forcedSquareDim * 0.16f; 
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

void checkGlobalClosePopupDismiss(float touchX, float touchY) {
    bool activePopup = (gameUI.isHintPopupActive || gameUI.isThemePopupActive || gameUI.isRatingPopupActive);
    if (!activePopup) return;

    float dW = gameUI.screenWidth * 0.76f;
    float dH = gameUI.screenHeight * 0.32f;
    float dX = (gameUI.screenWidth - dW) / 2.0f;
    float dY = (gameUI.screenHeight - dH) / 2.0f;

    float uniformCloseSize = 90.0f; 
    float uniformCloseX = dX + dW - uniformCloseSize - 20.0f;
    float uniformCloseY = dY + 20.0f;

    if (touchX >= uniformCloseX && touchX <= (uniformCloseX + uniformCloseSize) &&
        touchY >= uniformCloseY && touchY <= (uniformCloseY + uniformCloseSize)) {
        gameUI.isHintPopupActive = false;
        gameUI.isThemePopupActive = false;
        gameUI.isRatingPopupActive = false;
    }
}

extern "C" {

JNIEXPORT void JNICALL
Java_com_night_backgroundchange_MainActivity_nativeRender(JNIEnv* env, jobject obj, jobject canvas) {
    if (!canvas || !gameUI.engineInitialized || !gameUI.midDrawColor) return;

    int baseBgColor = gameUI.isCurrentlyDark ? 0xFF121212 : 0xFFFFFFFF;
    int baseTxtColor = gameUI.isCurrentlyDark ? 0xFFFFFFFF : 0xFF000000;
    int activeSelectionColor = gameUI.isCurrentlyDark ? 0xFFFFFFFF : 0xFF000000;
    int unselectedGrayColor = 0xFF7A7A7A;

    env->CallVoidMethod(canvas, gameUI.midDrawColor, baseBgColor);
    gameUI.UIButtons.clear();

    jobject tintActive = getTintPaint(env, obj, activeSelectionColor);
    jobject tintGray = getTintPaint(env, obj, unselectedGrayColor);
    jobject tintRed = getTintPaint(env, obj, 0xFFFF3B30);

    float headerFixedBarHeight = 160.0f;
    float footerFixedBarHeight = 160.0f;
    float footerStartY = gameUI.screenHeight - footerFixedBarHeight;

    // --- 1. RENDER MAIN VIEWS ---
    if (gameUI.currentState == STATE_HOME) {
        drawGameHeader(env, obj, canvas, baseBgColor, baseTxtColor, tintRed);

        float removeAdsX = gameUI.screenWidth - 110.0f;
        float removeAdsY = 50.0f;
        if (gameUI.assetBitmaps[ASSET_REMOVE_ADS]) {
            renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_REMOVE_ADS], removeAdsX, removeAdsY, 70.0f, tintActive);
            gameUI.UIButtons.push_back({removeAdsX, removeAdsY, 70.0f, 70.0f, 2010, 0});
        }

        // Reduced scaling size on Play button so it isn't huge
        float originalPlayWidth = gameUI.screenWidth * 0.32f;
        float playX = (gameUI.screenWidth / 2.0f) - (originalPlayWidth / 2.0f);
        float playY = gameUI.screenHeight * 0.44f;
        
        if (gameUI.assetBitmaps[ASSET_PLAY]) {
            renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_PLAY], playX, playY, originalPlayWidth, tintActive);
            gameUI.UIButtons.push_back({playX, playY, originalPlayWidth, originalPlayWidth, 2001, 0});
        }
    }

    if (gameUI.currentState == STATE_LEVELS) {
        float boxSize = gameUI.screenWidth * 0.24f;
        float spaceGrid = gameUI.screenWidth * 0.04f;
        float offsetGridX = (gameUI.screenWidth - (3 * boxSize + 2 * spaceGrid)) / 2.0f;
        
        int totalRows = (int)std::ceil(50.0f / 3.0f);
        float totalContentHeight = totalRows * (boxSize + spaceGrid) + 60.0f;
        gameUI.maxScrollExtent = totalContentHeight - (footerStartY - headerFixedBarHeight);
        if (gameUI.maxScrollExtent < 0.0f) gameUI.maxScrollExtent = 0.0f;

        env->CallIntMethod(canvas, gameUI.midSave);
        
        for (int i = 0; i < 50; i++) {
            int row = i / 3;
            int col = i % 3;
            float bx = offsetGridX + col * (boxSize + spaceGrid);
            float by = headerFixedBarHeight + 20.0f + row * (boxSize + spaceGrid) + gameUI.levelScrollOffset;

            // Strict visual filtering block: Do not draw outside header and footer bar
            if (by + boxSize < headerFixedBarHeight || by > footerStartY) continue;

            drawRealShadowRoundRect(env, canvas, bx, by, bx + boxSize, by + boxSize, 20, 20);
            int finalBoxColor = gameUI.levelsUnlocked[i] ? 0xFF007AFF : 0xFFD3D3D3;
            drawRoundRectNative(env, canvas, bx, by, bx + boxSize, by + boxSize, 20, 20, finalBoxColor);

            if (!gameUI.levelsUnlocked[i]) {
                if (gameUI.assetBitmaps[ASSET_LOCK]) {
                    renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_LOCK], bx + (boxSize * 0.30f), by + (boxSize * 0.30f), boxSize * 0.40f, tintActive);
                }
            } else {
                setPaintFontWeight(env, gameUI.paintTextReference, true);
                if (gameUI.paintTextReference) {
                    jclass paintCls = env->GetObjectClass(gameUI.paintTextReference);
                    jmethodID setTextSize = env->GetMethodID(paintCls, "setTextSize", "(F)V");
                    env->CallVoidMethod(gameUI.paintTextReference, setTextSize, 40.0f);
                }
                std::string numLvl = std::to_string(i + 1);
                jstring jNumL = env->NewStringUTF(numLvl.c_str());
                env->CallVoidMethod(canvas, gameUI.midDrawText, jNumL, bx + (boxSize * 0.38f), by + (boxSize * 0.62f), gameUI.paintTextReference);
                env->DeleteLocalRef(jNumL);
                setPaintFontWeight(env, gameUI.paintTextReference, false);
            }

            gameUI.UIButtons.push_back({bx, by, boxSize, boxSize, 3000 + i, i});
        }
        env->CallVoidMethod(canvas, gameUI.midRestore);

        // --- FIXED TOP HEADER BAR FOR SELECT LEVEL PAGE ---
        drawRoundRectNative(env, canvas, 0, 0, gameUI.screenWidth, headerFixedBarHeight, 0, 0, baseBgColor);
        setPaintFontWeight(env, gameUI.paintTextReference, true);
        if (gameUI.paintTextReference) {
            jclass paintCls = env->GetObjectClass(gameUI.paintTextReference);
            jmethodID setTextSize = env->GetMethodID(paintCls, "setTextSize", "(F)V");
            env->CallVoidMethod(gameUI.paintTextReference, setTextSize, 55.0f);
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

    if (gameUI.currentState == STATE_GAMEPLAY) {
        float headerIconSize = gameUI.screenWidth * 0.09f;
        float baseIconY = 35.0f;

        if (gameUI.assetBitmaps[ASSET_PAUSED]) {
            renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_PAUSED], 40.0f, baseIconY, headerIconSize, tintGray);
            gameUI.UIButtons.push_back({40.0f, baseIconY, headerIconSize, headerIconSize, 4002, 0});
        }
    }

    // --- 2. FIXED FOOTER AND NAVIGATION ARCHITECTURE ---
    if (gameUI.currentState == STATE_HOME || gameUI.currentState == STATE_SETTINGS || gameUI.currentState == STATE_LEVELS) {
        drawRoundRectNative(env, canvas, 0, footerStartY, gameUI.screenWidth, gameUI.screenHeight, 0, 0, gameUI.isCurrentlyDark ? 0xFF1E1E1E : 0xFFF8F9FA);

        float navIconSize = 65.0f; // Scale down navbar icons cleanly
        float innerSpaceY = footerStartY + (footerFixedBarHeight / 2.0f) - (navIconSize / 2.0f);
        
        // 3-Column Navigation layout step coordinates
        float sectionStep = gameUI.screenWidth / 3.0f;

        // Position 1: Home Button
        if (gameUI.assetBitmaps[ASSET_HOME]) {
            jobject homePaint = (gameUI.currentState == STATE_HOME) ? tintActive : tintGray;
            float homeX = (sectionStep * 0.5f) - (navIconSize / 2.0f);
            renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_HOME], homeX, innerSpaceY, navIconSize, homePaint);
            gameUI.UIButtons.push_back({homeX, innerSpaceY, navIconSize, navIconSize, 9001, 0});
        }

        // Position 2: Levels Selection Button
        if (gameUI.assetBitmaps[ASSET_LEVEL]) {
            jobject lvlPaint = (gameUI.currentState == STATE_LEVELS) ? tintActive : tintGray;
            float lvlX = (sectionStep * 1.5f) - (navIconSize / 2.0f);
            renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_LEVEL], lvlX, innerSpaceY, navIconSize, lvlPaint);
            gameUI.UIButtons.push_back({lvlX, innerSpaceY, navIconSize, navIconSize, 9002, 0});
        }

        // Position 3: Dynamic Settings Configuration Button Layout Added!
        if (gameUI.assetBitmaps[ASSET_SETTINGS]) {
            jobject setPaint = (gameUI.currentState == STATE_SETTINGS) ? tintActive : tintGray;
            float setX = (sectionStep * 2.5f) - (navIconSize / 2.0f);
            renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_SETTINGS], setX, innerSpaceY, navIconSize, setPaint);
            gameUI.UIButtons.push_back({setX, innerSpaceY, navIconSize, navIconSize, 9003, 0});
        }
    }

    // --- 3. DIALOG OVERLAY POPUPS ---
    bool activeModalBlocks = (gameUI.isHintPopupActive || gameUI.isThemePopupActive || gameUI.isRatingPopupActive || gameUI.isPausePopupActive);

    if (activeModalBlocks) {
        drawRoundRectNative(env, canvas, 0, 0, gameUI.screenWidth, gameUI.screenHeight, 0, 0, 0x66000000);

        float dW = gameUI.screenWidth * 0.76f;
        float dH = gameUI.screenHeight * 0.32f;
        float dX = (gameUI.screenWidth - dW) / 2.0f;
        float dY = (gameUI.screenHeight - dH) / 2.0f;

        if (gameUI.isPausePopupActive) {
            drawHorizontalPausePopup(env, canvas, dX, dY, dW, gameUI.screenHeight * 0.44f, tintActive);
        } else {
            drawRoundRectNative(env, canvas, dX, dY, dX + dW, dY + dH, 36, 36, gameUI.isCurrentlyDark ? 0xFF1E1E1E : 0xFFFFFFFF);

            float uniformCloseSize = 60.0f; 
            float uniformCloseX = dX + dW - uniformCloseSize - 25.0f;
            float uniformCloseY = dY + 25.0f;

            if (gameUI.assetBitmaps[ASSET_CLOSE]) {
                renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_CLOSE], uniformCloseX, uniformCloseY, uniformCloseSize, tintActive);
                gameUI.UIButtons.push_back({uniformCloseX - 10, uniformCloseY - 10, uniformCloseSize + 20, uniformCloseSize + 20, 9999, 0});
            }
        }
    }
}

} // extern "C"
