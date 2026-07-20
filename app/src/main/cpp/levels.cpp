#include "game_structures.h"
#include <algorithm>
#include <cmath>

void drawLevelSelection(JNIEnv* env, jobject canvas) {
    if (gameUI.currentState != STATE_LEVELS) return;

    // 1. Setup Grid Parameters
    int totalLevels = 15; // Set to your total number of levels
    int cols = 3;
    float cardSize = gameUI.screenWidth * 0.25f;
    float spacing = gameUI.screenWidth * 0.05f;
    
    float totalGridWidth = (cols * cardSize) + ((cols - 1) * spacing);
    float startX = (gameUI.screenWidth - totalGridWidth) / 2.0f;
    float startY = gameUI.screenHeight * 0.15f; // Starts below header

    // 2. Set Scroll Boundaries Dynamically
    int rows = std::ceil((float)totalLevels / cols);
    float totalGridHeight = (rows * cardSize) + ((rows - 1) * spacing);
    float visibleAreaHeight = gameUI.screenHeight * 0.70f;
    
    gameUI.maxScrollExtent = std::max(0.0f, totalGridHeight - visibleAreaHeight);

    // 3. Render Level Grid
    for (int i = 0; i < totalLevels; ++i) {
        int row = i / cols;
        int col = i % cols;

        float btnX = startX + col * (cardSize + spacing);
        float btnY = startY + row * (cardSize + spacing) + gameUI.levelScrollOffset;

        // Viewport check: Only draw and register touches if within main screen area (above bottom bar)
        if (btnY >= (gameUI.screenHeight * 0.10f) && (btnY + cardSize) <= (gameUI.screenHeight * 0.85f)) {
            
            // Draw background card (Replace paint color with your active paint object)
            drawRoundRectNative(env, canvas, btnX, btnY, btnX + cardSize, btnY + cardSize, 16, 16, gameUI.isCurrentlyDark ? 0xFF2A2E3D : 0xFFE0E0E0);

            // Register touch target
            // Action code 7000 + index (or your specific level action code)
            gameUI.UIButtons.push_back({btnX, btnY, cardSize, cardSize, 7000 + i, i});
        }
    }
}
