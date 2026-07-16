#include "game_structures.h"
#include <android/log.h>

// Fix #6: Heading Layout Fix ("RUN ARROW" centered header with integrated vertical Arrow 'A')
void drawGameHeader(JNIEnv* env, jobject obj, jobject canvas, int baseBgColor, int baseTxtColor, jobject tintRed) {
    setPaintFontWeight(env, gameUI.paintTextReference, true);
    if (gameUI.paintTextReference) {
        jclass paintCls = env->GetObjectClass(gameUI.paintTextReference);
        jmethodID setTextSize = env->GetMethodID(paintCls, "setTextSize", "(F)V");
        env->CallVoidMethod(gameUI.paintTextReference, setTextSize, 85.0f);
    }

    float midPointX = gameUI.screenWidth / 2.0f;
    float fixedHeaderY = 150.0f;

    // Split render anchors to center position text
    jstring titleR = env->NewStringUTF("RUN   RROW");
    env->CallVoidMethod(canvas, gameUI.midDrawText, titleR, midPointX - 250.0f, fixedHeaderY, gameUI.paintTextReference);
    env->DeleteLocalRef(titleR);

    // Render replacement directional arrow asset where the letter 'A' sits
    float headerArrowSize = 85.0f;
    float headerArrowX = midPointX + 18.0f;
    float headerArrowY = fixedHeaderY - 72.0f;

    if (gameUI.assetBitmaps[ASSET_ARROW]) {
        jclass canvasCls = env->GetObjectClass(canvas);
        jmethodID midRotate = env->GetMethodID(canvasCls, "rotate", "(FFF)V");
        
        env->CallIntMethod(canvas, gameUI.midSave);
        // Rotate asset 270 degrees to make it point cleanly upward
        if (midRotate) env->CallVoidMethod(canvas, midRotate, 270.0f, headerArrowX + (headerArrowSize / 2.0f), headerArrowY + (headerArrowSize / 2.0f));
        renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_ARROW], headerArrowX, headerArrowY, headerArrowSize, tintRed);
        env->CallVoidMethod(canvas, gameUI.midRestore);
    }
}

// Fix #5: Large, 90° Anticlockwise Rotated Light Gray Watermark
void drawWatermark(JNIEnv* env, jobject canvas) {
    if (!gameUI.paintTextReference) return;
    
    jclass paintCls = env->GetObjectClass(gameUI.paintTextReference);
    jmethodID setColor = env->GetMethodID(paintCls, "setColor", "(I)V");
    jmethodID setTextSize = env->GetMethodID(paintCls, "setTextSize", "(F)V");
    
    env->CallIntMethod(canvas, gameUI.midSave);
    
    // Set text parameters to a soft, light gray color mask
    env->CallVoidMethod(gameUI.paintTextReference, setColor, gameUI.isCurrentlyDark ? 0x1AFFFFFF : 0x11000000);
    env->CallVoidMethod(gameUI.paintTextReference, setTextSize, 120.0f);
    setPaintFontWeight(env, gameUI.paintTextReference, true);

    jstring wmString = env->NewStringUTF("RUN ARROW ENGINE");
    jclass canvasCls = env->GetObjectClass(canvas);
    jmethodID midRotate = env->GetMethodID(canvasCls, "rotate", "(FFF)V");
    
    // Rotate 90 degrees anticlockwise around layout boundaries
    if (midRotate) env->CallVoidMethod(canvas, midRotate, -90.0f, gameUI.screenWidth / 2.0f, gameUI.screenHeight / 2.0f);
    
    env->CallVoidMethod(canvas, gameUI.midDrawText, wmString, gameUI.screenWidth * 0.15f, gameUI.screenHeight / 2.0f, gameUI.paintTextReference);
    
    env->DeleteLocalRef(wmString);
    env->CallVoidMethod(canvas, gameUI.midRestore);
}

// Fix #4: Aspect-Ratio Locked Square Horizontal Gameplay Pause Container
void drawHorizontalPausePopup(JNIEnv* env, jobject canvas, float dX, float dY, float dW, float dH, jobject tintActive) {
    // Dynamically lock proportions so Width matches Height exactly (Square structure)
    float forcedSquareDim = dW > dH ? dH : dW;
    float squareLeft = dX + (dW - forcedSquareDim) / 2.0f;
    float squareTop = dY + (dH - forcedSquareDim) / 2.0f;

    // Draw clean background square box container without text items
    drawRoundRectNative(env, canvas, squareLeft, squareTop, squareLeft + forcedSquareDim, squareTop + forcedSquareDim, 36, 36, gameUI.isCurrentlyDark ? 0xFF1E1E1E : 0xFFFFFFFF);

    // Arrange elements horizontally inside the container
    float buttonSize = forcedSquareDim * 0.20f; 
    float innerSpacingY = squareTop + (forcedSquareDim / 2.0f) - (buttonSize / 2.0f);
    float itemHorizontalStep = forcedSquareDim / 4.0f;

    // Item 1: Large Play Icon
    float btn1X = squareLeft + itemHorizontalStep - (buttonSize / 2.0f);
    if (gameUI.assetBitmaps[ASSET_PLAY]) {
        renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_PLAY], btn1X, innerSpacingY, buttonSize, tintActive);
    }
    gameUI.UIButtons.push_back({btn1X, innerSpacingY, buttonSize, buttonSize, 5501, 0});

    // Item 2: Large Retry Icon
    float btn2X = squareLeft + (itemHorizontalStep * 2.0f) - (buttonSize / 2.0f);
    if (gameUI.assetBitmaps[ASSET_RETRY]) {
        renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_RETRY], btn2X, innerSpacingY, buttonSize, tintActive);
    }
    gameUI.UIButtons.push_back({btn2X, innerSpacingY, buttonSize, buttonSize, 5502, 0});

    // Item 3: Large Home/Menu Icon
    float btn3X = squareLeft + (itemHorizontalStep * 3.0f) - (buttonSize / 2.0f);
    if (gameUI.assetBitmaps[ASSET_HOME]) {
        renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_HOME], btn3X, innerSpacingY, buttonSize, tintActive);
    }
    gameUI.UIButtons.push_back({btn3X, innerSpacingY, buttonSize, buttonSize, 5504, 0});
}

// Fix #1: Global Close Target Engine Interface Layer
void checkGlobalClosePopupDismiss(float touchX, float touchY) {
    bool activePopup = (gameUI.isHintPopupActive || gameUI.isThemePopupActive || gameUI.isRatingPopupActive);
    if (!activePopup) return;

    float dW = gameUI.screenWidth * 0.76f;
    float dH = gameUI.screenHeight * 0.32f;
    float dX = (gameUI.screenWidth - dW) / 2.0f;
    float dY = (gameUI.screenHeight - dH) / 2.0f;

    float uniformCloseSize = 120.0f; // Enlarged touch target box footprint
    float uniformCloseX = dX + dW - uniformCloseSize - 10.0f;
    float uniformCloseY = dY + 10.0f;

    // If a touch falls anywhere inside the clean bounds, close the active screen layer smoothly
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
    jobject tintYellow = getTintPaint(env, obj, 0xFFFFCC00);
    jobject tintRed = getTintPaint(env, obj, 0xFFFF3B30);

    // Fix #7: Header and Footer Layout Configuration Anchors
    float headerFixedBarHeight = 180.0f;
    float footerFixedBarHeight = 200.0f;
    float footerStartY = gameUI.screenHeight - footerFixedBarHeight;

    // Draw the continuous background watermark pattern layer beneath elements
    drawWatermark(env, canvas);

    if (gameUI.currentState == STATE_HOME) {
        // Fix #6: Render Heading Layout Block
        drawGameHeader(env, obj, canvas, baseBgColor, baseTxtColor, tintRed);

        // Position the dynamic header ad button
        float removeAdsX = gameUI.screenWidth - 140.0f;
        float removeAdsY = 65.0f;
        if (gameUI.assetBitmaps[ASSET_REMOVE_ADS]) {
            renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_REMOVE_ADS], removeAdsX, removeAdsY, 90.0f, tintActive);
            gameUI.UIButtons.push_back({removeAdsX, removeAdsY, 90.0f, 90.0f, 2010, 0});
        }

        // Fix #2: Scaled Play Button Layout Modification (Reduced by 25% cleanly)
        float originalPlayWidth = gameUI.screenWidth * 0.45f;
        float scaledPlayWidth = originalPlayWidth * 0.75f;
        float playX = (gameUI.screenWidth / 2.0f) - (scaledPlayWidth / 2.0f);
        float playY = gameUI.screenHeight * 0.48f;
        
        if (gameUI.assetBitmaps[ASSET_PLAY]) {
            renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_PLAY], playX, playY, scaledPlayWidth, tintActive);
            gameUI.UIButtons.push_back({playX, playY, scaledPlayWidth, scaledPlayWidth, 2001, 0});
        }
    }

    if (gameUI.currentState == STATE_LEVELS) {
        // Render Fixed Page Header Label Layer above content views
        drawRoundRectNative(env, canvas, 0, 0, gameUI.screenWidth, headerFixedBarHeight, 0, 0, baseBgColor);
        setPaintFontWeight(env, gameUI.paintTextReference, true);
        jstring levelHeader = env->NewStringUTF("SELECT LEVEL");
        env->CallVoidMethod(canvas, gameUI.midDrawText, levelHeader, (jfloat)(gameUI.screenWidth * 0.32f), 110.0f, gameUI.paintTextReference);
        env->DeleteLocalRef(levelHeader);
        setPaintFontWeight(env, gameUI.paintTextReference, false);

        float boxSize = gameUI.screenWidth * 0.25f;
        float spaceGrid = gameUI.screenWidth * 0.045f;
        float offsetGridX = (gameUI.screenWidth - (3 * boxSize + 2 * spaceGrid)) / 2.0f;
        
        int totalRows = (int)std::ceil(50.0f / 3.0f);
        float totalContentHeight = totalRows * (boxSize + spaceGrid) + 100.0f;
        gameUI.maxScrollExtent = totalContentHeight - (footerStartY - headerFixedBarHeight);
        if (gameUI.maxScrollExtent < 0.0f) gameUI.maxScrollExtent = 0.0f;

        // Fix #7: Clip the scrolling layout grid between header and footer boundaries
        env->CallIntMethod(canvas, gameUI.midSave);
        
        for (int i = 0; i < 50; i++) {
            int row = i / 3;
            int col = i % 3;
            float bx = offsetGridX + col * (boxSize + spaceGrid);
            float by = headerFixedBarHeight + 30.0f + row * (boxSize + spaceGrid) + gameUI.levelScrollOffset;

            // Cull offscreen components outside the scrolling bounds layout frame
            if (by + boxSize < headerFixedBarHeight || by > footerStartY) continue;

            drawRealShadowRoundRect(env, canvas, bx, by, bx + boxSize, by + boxSize, 24, 24);
            int finalBoxColor = gameUI.levelsUnlocked[i] ? 0xFF007AFF : 0xFFD3D3D3;
            drawRoundRectNative(env, canvas, bx, by, bx + boxSize, by + boxSize, 24, 24, finalBoxColor);

            if (!gameUI.levelsUnlocked[i]) {
                if (gameUI.assetBitmaps[ASSET_LOCK]) {
                    renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_LOCK], bx + (boxSize * 0.28f), by + (boxSize * 0.28f), boxSize * 0.44f, tintActive);
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
        env->CallVoidMethod(canvas, gameUI.midRestore);
    }

    if (gameUI.currentState == STATE_GAMEPLAY) {
        float headerIconSize = gameUI.screenWidth * 0.11f;
        float baseIconY = 45.0f;

        if (gameUI.assetBitmaps[ASSET_PAUSED]) {
            renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_PAUSED], 40.0f, baseIconY, headerIconSize, tintGray);
            gameUI.UIButtons.push_back({40.0f, baseIconY, headerIconSize, headerIconSize, 4002, 0});
        }
    }

    // Fix #7: Render the Fixed Navigation Footer Panel Layer cleanly over content lists
    if (gameUI.currentState == STATE_HOME || gameUI.currentState == STATE_SETTINGS || gameUI.currentState == STATE_LEVELS) {
        drawRoundRectNative(env, canvas, 0, footerStartY, gameUI.screenWidth, gameUI.screenHeight, 0, 0, gameUI.isCurrentlyDark ? 0xFF1E1E1E : 0xFFF8F9FA);

        float navIconSize = gameUI.screenWidth * 0.11f;
        float paddingEdge = 65.0f;
        float innerSpaceY = footerStartY + (footerFixedBarHeight / 2.0f) - (navIconSize / 2.0f);

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
    }

    // Overlay Popup Modal Rendering Engines
    bool activeModalBlocks = (gameUI.isHintPopupActive || gameUI.isThemePopupActive || gameUI.isRatingPopupActive || gameUI.isPausePopupActive);

    if (activeModalBlocks) {
        drawRoundRectNative(env, canvas, 0, 0, gameUI.screenWidth, gameUI.screenHeight, 0, 0, 0x66000000);

        float dW = gameUI.screenWidth * 0.76f;
        float dH = gameUI.screenHeight * 0.32f;
        float dX = (gameUI.screenWidth - dW) / 2.0f;
        float dY = (gameUI.screenHeight - dH) / 2.0f;

        // Fix #4: Forward Pause Popup structural measurements directly into specialized layout systems
        if (gameUI.isPausePopupActive) {
            drawHorizontalPausePopup(env, canvas, dX, dY, dW, gameUI.screenHeight * 0.44f, tintActive);
        } else {
            // Standard Popup Container Box Model UI Render Core logic
            drawRoundRectNative(env, canvas, dX, dY, dX + dW, dY + dH, 36, 36, gameUI.isCurrentlyDark ? 0xFF1E1E1E : 0xFFFFFFFF);

            // Fix #1: Clean top-right integrated closing target item asset configuration
            float uniformCloseSize = 75.0f; 
            float uniformCloseX = dX + dW - uniformCloseSize - 30.0f;
            float uniformCloseY = dY + 30.0f;

            if (gameUI.assetBitmaps[ASSET_CLOSE]) {
                renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_CLOSE], uniformCloseX, uniformCloseY, uniformCloseSize, tintActive);
                gameUI.UIButtons.push_back({uniformCloseX - 10, uniformCloseY - 10, uniformCloseSize + 20, uniformCloseSize + 20, 9999, 0});
            }
        }
    }
}

} // extern "C"
