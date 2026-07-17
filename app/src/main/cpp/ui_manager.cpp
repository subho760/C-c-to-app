#include "game_structures.h"
#include <cmath>
#include <string>

// Helper to draw clean rounded buttons
void drawDialogButton(JNIEnv* env, jobject canvas, float x, float y, float w, float h, const char* label, int bgColor, int textColor) {
    drawRoundRectNative(env, canvas, x, y, x + w, y + h, 15, 15, bgColor);
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
        float ty = y + (h / 2.0f) + 11.0f; 
        env->CallVoidMethod(canvas, gameUI.midDrawText, jText, tx, ty, gameUI.paintTextReference);
        env->DeleteLocalRef(jText);
        setPaintFontWeight(env, gameUI.paintTextReference, false);
    }
}

// Cleans up headers and replaces the 'A' in Arrow with the red arrow PNG
void drawGameHeader(JNIEnv* env, jobject obj, jobject canvas, int baseBgColor, int baseTxtColor, jobject tintRed) {
    if (!gameUI.paintTextReference) return;
    
    setPaintFontWeight(env, gameUI.paintTextReference, true);
    jclass paintCls = env->GetObjectClass(gameUI.paintTextReference);
    jmethodID setTextSize = env->GetMethodID(paintCls, "setTextSize", "(F)V");
    jmethodID setColor = env->GetMethodID(paintCls, "setColor", "(I)V");
    jmethodID measureText = env->GetMethodID(paintCls, "measureText", "(Ljava/lang/String;)F");
    
    env->CallVoidMethod(gameUI.paintTextReference, setTextSize, 65.0f);
    env->CallVoidMethod(gameUI.paintTextReference, setColor, baseTxtColor);

    float midPointX = gameUI.screenWidth / 2.0f;
    float fixedHeaderY = 130.0f;

    jstring runStr = env->NewStringUTF("RUN ");
    jstring rrowStr = env->NewStringUTF("RROW");

    float runWidth = env->CallFloatMethod(gameUI.paintTextReference, measureText, runStr);
    float rrowWidth = env->CallFloatMethod(gameUI.paintTextReference, measureText, rrowStr);
    
    float inlineArrowSize = 50.0f;
    float totalHeaderWidth = runWidth + inlineArrowSize + rrowWidth + 10.0f;
    float startX = midPointX - (totalHeaderWidth / 2.0f);

    env->CallVoidMethod(canvas, gameUI.midDrawText, runStr, startX, fixedHeaderY, gameUI.paintTextReference);

    float arrowX = startX + runWidth;
    float arrowY = fixedHeaderY - 48.0f;
    if (gameUI.assetBitmaps[ASSET_ARROW]) {
        renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_ARROW], arrowX, arrowY, inlineArrowSize, tintRed);
    }

    float rrowX = arrowX + inlineArrowSize + 10.0f;
    env->CallVoidMethod(canvas, gameUI.midDrawText, rrowStr, rrowX, fixedHeaderY, gameUI.paintTextReference);

    env->DeleteLocalRef(runStr);
    env->DeleteLocalRef(rrowStr);
    setPaintFontWeight(env, gameUI.paintTextReference, false);
}

// Re-added background arrow watermark layout pattern covering the center frame safely
void drawWatermark(JNIEnv* env, jobject canvas) {
    if (gameUI.assetBitmaps[ASSET_ARROW]) {
        // Render a large, faded background watermark in the center of the playground layout
        jclass paintCls = env->FindClass("android/graphics/Paint");
        jmethodID paintInit = env->GetMethodID(paintCls, "<init>", "()V");
        jobject watermarkPaint = env->NewObject(paintCls, paintInit);
        jmethodID setAlpha = env->GetMethodID(paintCls, "setAlpha", "(I)V");
        env->CallVoidMethod(watermarkPaint, setAlpha, 30); // Low opacity overlay (approx 12%)

        float watermarkSize = gameUI.screenWidth * 0.55f;
        float wx = (gameUI.screenWidth - watermarkSize) / 2.0f;
        float wy = (gameUI.screenHeight - watermarkSize) / 2.0f;
        
        renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_ARROW], wx, wy, watermarkSize, watermarkPaint);
        env->DeleteLocalRef(watermarkPaint);
        env->DeleteLocalRef(paintCls);
    }
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
    if (gameUI.isHintPopupActive || gameUI.isThemePopupActive || gameUI.isRatingPopupActive) {
        float dW = gameUI.screenWidth * 0.76f;
        float dH = gameUI.screenHeight * 0.32f;
        float dX = (gameUI.screenWidth - dW) / 2.0f;
        float dY = (gameUI.screenHeight - dH) / 2.0f;

        float uniformCloseSize = 60.0f; 
        float uniformCloseX = dX + dW - uniformCloseSize - 20.0f;
        float uniformCloseY = dY + 20.0f;

        if (touchX >= uniformCloseX && touchX <= (uniformCloseX + uniformCloseSize) &&
            touchY >= uniformCloseY && touchY <= (uniformCloseY + uniformCloseSize)) {
            gameUI.isHintPopupActive = false;
            gameUI.isThemePopupActive = false;
            gameUI.isRatingPopupActive = false;
        }
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

    // --- 1. HOME SCREEN ---
    if (gameUI.currentState == STATE_HOME) {
        drawGameHeader(env, obj, canvas, baseBgColor, baseTxtColor, tintRed);

        float removeAdsX = gameUI.screenWidth - 110.0f;
        float removeAdsY = 50.0f;
        if (gameUI.assetBitmaps[ASSET_REMOVE_ADS]) {
            renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_REMOVE_ADS], removeAdsX, removeAdsY, 70.0f, tintActive);
            gameUI.UIButtons.push_back({removeAdsX, removeAdsY, 70.0f, 70.0f, 2010, 0});
        }

        float originalPlayWidth = gameUI.screenWidth * 0.32f;
        float playX = (gameUI.screenWidth / 2.0f) - (originalPlayWidth / 2.0f);
        float playY = gameUI.screenHeight * 0.44f;
        
        if (gameUI.assetBitmaps[ASSET_PLAY]) {
            renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_PLAY], playX, playY, originalPlayWidth, tintActive);
            gameUI.UIButtons.push_back({playX, playY, originalPlayWidth, originalPlayWidth, 2001, 0});
        }
    }

    // --- 2. SELECT LEVEL SCREEN ---
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

            // Tapping on any locked item instantly calls code 3500 which unlocks ALL levels now!
            int interactionCode = gameUI.levelsUnlocked[i] ? (3000 + i) : 3500;
            gameUI.UIButtons.push_back({bx, by, boxSize, boxSize, interactionCode, i});
        }
        env->CallVoidMethod(canvas, gameUI.midRestore);

        // Fixed Top Header Panel
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

    // --- 3. SETTINGS MENU VIEW ---
    if (gameUI.currentState == STATE_SETTINGS) {
        drawRoundRectNative(env, canvas, 0, 0, gameUI.screenWidth, headerFixedBarHeight, 0, 0, baseBgColor);
        if (gameUI.paintTextReference) {
            setPaintFontWeight(env, gameUI.paintTextReference, true);
            jclass paintCls = env->GetObjectClass(gameUI.paintTextReference);
            jmethodID setTextSize = env->GetMethodID(paintCls, "setTextSize", "(F)V");
            env->CallVoidMethod(gameUI.paintTextReference, setTextSize, 55.0f);
            jstring setHeader = env->NewStringUTF("SETTINGS");
            env->CallVoidMethod(canvas, gameUI.midDrawText, setHeader, 40.0f, 105.0f, gameUI.paintTextReference);
            env->DeleteLocalRef(setHeader);
            setPaintFontWeight(env, gameUI.paintTextReference, false);
        }

        float optionY = headerFixedBarHeight + 40.0f;
        float optionHeight = 100.0f;
        float optionSpacing = 30.0f;
        float marginX = 40.0f;
        float rowWidth = gameUI.screenWidth - (2 * marginX);

        // Option 2 (Rate My App) now triggers the Ad pop-up ("isRatingPopupActive") instead of opening a direct link
        const char* optionsNames[] = {"Change Theme", "Rate My App", "Share My App", "Privacy Policy"};
        int optionActions[] = {6501, 3555, 6503, 6502}; // Action 3555 activates ad dialog context

        for (int i = 0; i < 4; i++) {
            int rowColor = gameUI.isCurrentlyDark ? 0xFF1E1E1E : 0xFFF1F3F5;
            drawRoundRectNative(env, canvas, marginX, optionY, marginX + rowWidth, optionY + optionHeight, 18, 18, rowColor);

            if (gameUI.paintTextReference) {
                jclass paintCls = env->GetObjectClass(gameUI.paintTextReference);
                jmethodID setTextSize = env->GetMethodID(paintCls, "setTextSize", "(F)V");
                jmethodID setColor = env->GetMethodID(paintCls, "setColor", "(I)V");
                env->CallVoidMethod(gameUI.paintTextReference, setTextSize, 36.0f);
                env->CallVoidMethod(gameUI.paintTextReference, setColor, baseTxtColor);
                
                jstring textStr = env->NewStringUTF(optionsNames[i]);
                env->CallVoidMethod(canvas, gameUI.midDrawText, textStr, marginX + 30.0f, optionY + 60.0f, gameUI.paintTextReference);
                env->DeleteLocalRef(textStr);
            }

            gameUI.UIButtons.push_back({marginX, optionY, rowWidth, optionHeight, optionActions[i], 0});
            optionY += optionHeight + optionSpacing;
        }
    }

    // --- 4. GAMEPLAY PLAYGROUND SCREEN ---
    if (gameUI.currentState == STATE_GAMEPLAY) {
        // Render the requested arrow watermark backdrop layers safely inside the grid playing boundary
        drawWatermark(env, canvas);

        float headerIconSize = 65.0f;
        float baseIconY = 45.0f;

        drawRoundRectNative(env, canvas, 0, 0, gameUI.screenWidth, headerFixedBarHeight, 0, 0, baseBgColor);

        if (gameUI.assetBitmaps[ASSET_PAUSED]) {
            renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_PAUSED], 40.0f, baseIconY, headerIconSize, tintActive);
            gameUI.UIButtons.push_back({40.0f, baseIconY, headerIconSize, headerIconSize, 4002, 0});
        }

        if (gameUI.assetBitmaps[ASSET_HINT]) {
            float hintX = gameUI.screenWidth - headerIconSize - 40.0f;
            renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_HINT], hintX, baseIconY, headerIconSize, tintActive);
            gameUI.UIButtons.push_back({hintX, baseIconY, headerIconSize, headerIconSize, 4003, 0});
        }

        if (gameUI.paintTextReference) {
            setPaintFontWeight(env, gameUI.paintTextReference, true);
            jclass paintCls = env->GetObjectClass(gameUI.paintTextReference);
            jmethodID setTextSize = env->GetMethodID(paintCls, "setTextSize", "(F)V");
            jmethodID setColor = env->GetMethodID(paintCls, "setColor", "(I)V");
            jmethodID measureText = env->GetMethodID(paintCls, "measureText", "(Ljava/lang/String;)F");

            env->CallVoidMethod(gameUI.paintTextReference, setTextSize, 45.0f);
            env->CallVoidMethod(gameUI.paintTextReference, setColor, baseTxtColor);

            std::string lvlBanner = "LEVEL " + std::to_string(gameUI.currentPlayingLevel);
            jstring jBanner = env->NewStringUTF(lvlBanner.c_str());
            float textW = env->CallFloatMethod(gameUI.paintTextReference, measureText, jBanner);
            float bannerX = (gameUI.screenWidth / 2.0f) - (textW / 2.0f);

            env->CallVoidMethod(canvas, gameUI.midDrawText, jBanner, bannerX, 105.0f, gameUI.paintTextReference);
            env->DeleteLocalRef(jBanner);
            setPaintFontWeight(env, gameUI.paintTextReference, false);
        }

        // --- GENERATE GAMEPLAY MATRIX (50 DOTS CONFIGURATION GRID) ---
        int totalDotsCount = 50;
        int gridColumns = 5;
        int gridRows = 10; // 5 x 10 = 50 total game node points
        
        float startGridY = headerFixedBarHeight + 80.0f;
        float availableGridHeight = (gameUI.screenHeight - footerFixedBarHeight - 40.0f) - startGridY;
        float stepX = gameUI.screenWidth / (float)(gridColumns + 1);
        float stepY = availableGridHeight / (float)(gridRows + 1);

        float nodeCoordinatesX[50];
        float nodeCoordinatesY[50];

        // Draw the 50 gameplay dots matrix
        jclass nativePaintClass = env->GetObjectClass(gameUI.paintTextReference);
        jmethodID midDrawCircle = env->GetMethodID(env->GetObjectClass(canvas), "drawCircle", "(FFFLandroid/graphics/Paint;)V");
        jmethodID setPaintColor = env->GetMethodID(nativePaintClass, "setColor", "(I)V");

        env->CallVoidMethod(gameUI.paintTextReference, setPaintColor, gameUI.isCurrentlyDark ? 0xFFFFFFFF : 0xFF222222);

        int dotIndex = 0;
        for (int r = 1; r <= gridRows; r++) {
            for (int c = 1; c <= gridColumns; c++) {
                float circleX = c * stepX;
                float circleY = startGridY + (r * stepY);
                
                nodeCoordinatesX[dotIndex] = circleX;
                nodeCoordinatesY[dotIndex] = circleY;

                env->CallVoidMethod(canvas, midDrawCircle, circleX, circleY, 10.0f, gameUI.paintTextReference);
                dotIndex++;
            }
        }

        // Draw primary overlay arrow matching index 0 (first dot) to index 49 (last dot)
        if (gameUI.assetBitmaps[ASSET_ARROW]) {
            float fX = nodeCoordinatesX[0];
            float fY = nodeCoordinatesY[0];
            float lX = nodeCoordinatesX[49];
            float lY = nodeCoordinatesY[49];
            
            // Calculate scale constraints and directional target vector angle
            float deltaX = lX - fX;
            float deltaY = lY - fY;
            float lineLength = std::sqrt(deltaX * deltaX + deltaY * deltaY);
            float targetAngleDeg = std::atan2(deltaY, deltaX) * 180.0f / M_PI;

            env->CallIntMethod(canvas, gameUI.midSave);
            jmethodID midRotate = env->GetMethodID(env->GetObjectClass(canvas), "rotate", "(FFF)V");
            env->CallVoidMethod(canvas, midRotate, targetAngleDeg + 90.0f, fX, fY); // Native rotation alignment offset
            renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_ARROW], fX - 30.0f, fY, 60.0f, tintRed);
            env->CallVoidMethod(canvas, gameUI.midRestore);

            // Draw extra gameplay arrows at random locations to make it busy
            float extraPointsX[] = {nodeCoordinatesX[12], nodeCoordinatesX[23], nodeCoordinatesX[34]};
            float extraPointsY[] = {nodeCoordinatesY[12], nodeCoordinatesY[23], nodeCoordinatesY[34]};
            for (int k = 0; k < 3; k++) {
                renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_ARROW], extraPointsX[k] - 25.0f, extraPointsY[k] - 25.0f, 50.0f, tintActive);
            }
        }
    }

    // --- 5. FIXED FOOTER NAVIGATION ARCHITECTURE ---
    if (gameUI.currentState == STATE_HOME || gameUI.currentState == STATE_SETTINGS || gameUI.currentState == STATE_LEVELS) {
        drawRoundRectNative(env, canvas, 0, footerStartY, gameUI.screenWidth, gameUI.screenHeight, 0, 0, gameUI.isCurrentlyDark ? 0xFF1E1E1E : 0xFFF8F9FA);

        float navIconSize = 65.0f; 
        float innerSpaceY = footerStartY + (footerFixedBarHeight / 2.0f) - (navIconSize / 2.0f);
        float sectionStep = gameUI.screenWidth / 3.0f;

        if (gameUI.assetBitmaps[ASSET_HOME]) {
            jobject homePaint = (gameUI.currentState == STATE_HOME) ? tintActive : tintGray;
            float homeX = (sectionStep * 0.5f) - (navIconSize / 2.0f);
            renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_HOME], homeX, innerSpaceY, navIconSize, homePaint);
            gameUI.UIButtons.push_back({homeX, innerSpaceY, navIconSize, navIconSize, 9001, 0});
        }

        if (gameUI.assetBitmaps[ASSET_LEVEL]) {
            jobject lvlPaint = (gameUI.currentState == STATE_LEVELS) ? tintActive : tintGray;
            float lvlX = (sectionStep * 1.5f) - (navIconSize / 2.0f);
            renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_LEVEL], lvlX, innerSpaceY, navIconSize, lvlPaint);
            gameUI.UIButtons.push_back({lvlX, innerSpaceY, navIconSize, navIconSize, 9002, 0});
        }

        if (gameUI.assetBitmaps[ASSET_SETTINGS]) {
            jobject setPaint = (gameUI.currentState == STATE_SETTINGS) ? tintActive : tintGray;
            float setX = (sectionStep * 2.5f) - (navIconSize / 2.0f);
            renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_SETTINGS], setX, innerSpaceY, navIconSize, setPaint);
            gameUI.UIButtons.push_back({setX, innerSpaceY, navIconSize, navIconSize, 9003, 0});
        }
    }

    // --- 6. GLOBAL DIALOG POPUPS & INTERFACE INTERLAYS ---
    bool activeModalBlocks = (gameUI.isHintPopupActive || gameUI.isThemePopupActive || gameUI.isRatingPopupActive || gameUI.isPausePopupActive);

    if (activeModalBlocks) {
        drawRoundRectNative(env, canvas, 0, 0, gameUI.screenWidth, gameUI.screenHeight, 0, 0, 0x88000000);

        float dW = gameUI.screenWidth * 0.80f;
        float dH = gameUI.screenHeight * 0.35f;
        float dX = (gameUI.screenWidth - dW) / 2.0f;
        float dY = (gameUI.screenHeight - dH) / 2.0f;

        drawRoundRectNative(env, canvas, dX, dY, dX + dW, dY + dH, 36, 36, gameUI.isCurrentlyDark ? 0xFF1E1E1E : 0xFFFFFFFF);

        jclass paintCls = gameUI.paintTextReference ? env->GetObjectClass(gameUI.paintTextReference) : nullptr;
        jmethodID setTextSize = paintCls ? env->GetMethodID(paintCls, "setTextSize", "(F)V") : nullptr;
        jmethodID setColor = paintCls ? env->GetMethodID(paintCls, "setColor", "(I)V") : nullptr;
        jmethodID measureText = paintCls ? env->GetMethodID(paintCls, "measureText", "(Ljava/lang/String;)F") : nullptr;

        float closeBtnSize = 50.0f;
        float closeX = dX + dW - closeBtnSize - 25.0f;
        float closeY = dY + 25.0f;
        if (gameUI.assetBitmaps[ASSET_CLOSE]) {
            renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_CLOSE], closeX, closeY, closeBtnSize, tintActive);
            gameUI.UIButtons.push_back({closeX - 10.0f, closeY - 10.0f, closeBtnSize + 20.0f, closeBtnSize + 20.0f, 9999, 0});
        }

        // A. LEVEL UNLOCK CONFIRMATION DIALOG (Triggered via Rate option action or locked level)
        if (gameUI.isRatingPopupActive) { 
            if (paintCls) {
                setPaintFontWeight(env, gameUI.paintTextReference, true);
                env->CallVoidMethod(gameUI.paintTextReference, setTextSize, 38.0f);
                env->CallVoidMethod(gameUI.paintTextReference, setColor, baseTxtColor);
                
                jstring descText1 = env->NewStringUTF("For unlock this level");
                jstring descText2 = env->NewStringUTF("watch ads to continue");
                
                float tw1 = env->CallFloatMethod(gameUI.paintTextReference, measureText, descText1);
                float tw2 = env->CallFloatMethod(gameUI.paintTextReference, measureText, descText2);
                
                env->CallVoidMethod(canvas, gameUI.midDrawText, descText1, dX + (dW - tw1)/2.0f, dY + 95.0f, gameUI.paintTextReference);
                env->CallVoidMethod(canvas, gameUI.midDrawText, descText2, dX + (dW - tw2)/2.0f, dY + 145.0f, gameUI.paintTextReference);
                
                env->DeleteLocalRef(descText1);
                env->DeleteLocalRef(descText2);
                setPaintFontWeight(env, gameUI.paintTextReference, false);
            }

            float actionBtnW = dW * 0.70f;
            float actionBtnH = 85.0f;
            float actionX = dX + (dW - actionBtnW) / 2.0f;
            float actionY = dY + dH - actionBtnH - 35.0f;
            
            drawDialogButton(env, canvas, actionX, actionY, actionBtnW, actionBtnH, "  WATCH ADS", 0xFF007AFF, 0xFFFFFFFF);
            if (gameUI.assetBitmaps[ASSET_REMOVE_ADS]) {
                renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_REMOVE_ADS], actionX + 45.0f, actionY + 22.0f, 40.0f, tintActive);
            }
            // Triggers code 3500 which performs instant total unlock bypass sequence
            gameUI.UIButtons.push_back({actionX, actionY, actionBtnW, actionBtnH, 3500, 0});
        }

        // B. NEED HELP / LEVEL HINT OVERLAY DIALOG
        if (gameUI.isHintPopupActive) {
            if (paintCls) {
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
            
            drawDialogButton(env, canvas, actX, actY, actBtnW, actBtnH, "  WATCH ADS", 0xFFE2A000, 0xFFFFFFFF);
            if (gameUI.assetBitmaps[ASSET_REMOVE_ADS]) {
                renderBmp(env, canvas, gameUI.assetBitmaps[ASSET_REMOVE_ADS], actX + 45.0f, actY + 22.0f, 40.0f, tintActive);
            }
            gameUI.UIButtons.push_back({actX, actY, actBtnW, actBtnH, 4003, 0});
        }

        // C. INVERTED THEME MODAL (Light button activates dark mode, dark button activates light mode)
        if (gameUI.isThemePopupActive) {
            if (paintCls) {
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

            // Light Button triggers Dark Mode (Action 7701)
            float btnLightX = dX + 30.0f;
            drawDialogButton(env, canvas, btnLightX, rowY, colW, colH, "LIGHT", 0xFFE9ECEF, 0xFF000000);
            gameUI.UIButtons.push_back({btnLightX, rowY, colW, colH, 7701, 0});

            // Dark Button triggers Light Mode (Action 7700)
            float btnDarkX = dX + 40.0f + colW;
            drawDialogButton(env, canvas, btnDarkX, rowY, colW, colH, "DARK", 0xFF212529, 0xFFFFFFFF);
            gameUI.UIButtons.push_back({btnDarkX, rowY, colW, colH, 7700, 0});
        }

        // D. PAUSE DIALOG SYSTEM
        if (gameUI.isPausePopupActive) {
            drawHorizontalPausePopup(env, canvas, dX, dY, dW, gameUI.screenHeight * 0.44f, tintActive);
        }
    }
}

} // extern "C"
