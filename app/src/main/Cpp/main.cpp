#include <jni.h>
#include <string>
#include <cmath>

// Global game architecture state tracking
int g_gameState = 0; // 0 = Loading, 1 = Main Menu, 2 = Playing, 3 = GameOver
int g_playerLives = 3;
float g_playerX = 200.0f;
float g_playerY = 600.0f;
float g_targetX = 200.0f;
float g_targetY = 600.0f;

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
        // Linear interpolation step logic: smoothly glide player position to finger touch locations
        g_playerX += (g_targetX - g_playerX) * 0.12f;
        g_playerY += (g_targetY - g_playerY) * 0.12f;
        
        if (g_playerLives <= 0) {
            g_gameState = 3; // Shift engine mode to Game Over
        }
    }
}

JNIEXPORT void JNICALL
Java_com_night_backgroundchange_MainActivity_handleNativeTouch(JNIEnv* env, jobject thiz, jfloat x, jfloat y) {
    if (g_gameState == 1) {
        // Menu touch starts gameplay sequence
        g_gameState = 2; 
        g_playerX = x;
        g_playerY = y;
        g_targetX = x;
        g_targetY = y;
    } else if (g_gameState == 2) {
        // Track the path finger coordinates live
        g_targetX = x;
        g_targetY = y;
    } else if (g_gameState == 3) {
        // Reset full level data variables on retry touch execution
        g_playerLives = 3;
        g_playerX = 200.0f;
        g_playerY = 600.0f;
        g_targetX = 200.0f;
        g_targetY = 600.0f;
        g_gameState = 1; // Return to main menu structure
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
