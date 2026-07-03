#include <jni.h>
#include <string>
#include <cmath>

// Global game state variables
int g_gameState = 0; // 0 = Loading, 1 = Main Menu, 2 = Playing, 3 = GameOver
int g_playerLives = 3;
float g_playerX = 100.0f;
float g_playerY = 500.0f;
float g_targetX = 100.0f;
float g_targetY = 500.0f;

extern "C" {

JNIEXPORT jint JNICALL
Java_com_night_backgroundchange_MainActivity_getNativeGameState(JNIEnv* env, jobject thiz) {
    return g_gameState;
}

JNIEXPORT void JNICALL
Java_com_night_backgroundchange_MainActivity_setNativeGameState(JNIEnv* env, jobject thiz, jint state) {
    g_gameState = state;
}

JNIEXPORT jint JNICALL
Java_com_night_backgroundchange_MainActivity_getNativeLives(JNIEnv* env, jobject thiz) {
    return g_playerLives;
}

JNIEXPORT void JNICALL
Java_com_night_backgroundchange_MainActivity_updateNativeGame(JNIEnv* env, jobject thiz) {
    if (g_gameState == 2) {
        // Move player smoothly toward touch target location
        g_playerX += (g_targetX - g_playerX) * 0.1f;
        g_playerY += (g_targetY - g_playerY) * 0.1f;
        
        // Example logic: if lives hit 0, trigger game over
        if (g_playerLives <= 0) {
            g_gameState = 3; // Go to Game Over state
        }
    }
}

JNIEXPORT void JNICALL
Java_com_night_backgroundchange_MainActivity_handleNativeTouch(JNIEnv* env, jobject thiz, jfloat x, jfloat y) {
    if (g_gameState == 1) {
        // Main Menu: Simple check if they tapped screen center area to play
        g_gameState = 2; // Transition to Playing state
    } else if (g_gameState == 2) {
        // Update game target destination
        g_targetX = x;
        g_targetY = y;
    } else if (g_gameState == 3) {
        // Game Over screen tap resets game
        g_playerLives = 3;
        g_playerX = 100.0f;
        g_playerY = 500.0f;
        g_gameState = 1;
    }
}

JNIEXPORT jfloat JNICALL
Java_com_night_backgroundchange_MainActivity_getNativePlayerX(JNIEnv* env, jobject thiz) {
    return g_playerX;
}

JNIEXPORT jfloat JNICALL
Java_com_night_backgroundchange_MainActivity_getNativePlayerY(JNIEnv* env, jobject thiz) {
    return g_playerY;
}

}
