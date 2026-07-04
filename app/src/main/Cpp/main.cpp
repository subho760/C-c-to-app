#include <jni.h>
#include <string>
#include <cmath>

// Global Maze State Variables
int g_gameState = 0; // 0=Loading, 1=Menu, 2=Playing
int g_playerLives = 3;
int g_currentLevel = 92; // Matches your target level marker directly
float g_playerX = 0.0f;
float g_playerY = 0.0f;
float g_targetX = 0.0f;
float g_targetY = 0.0f;
int g_tickCounter = 0;

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

JNIEXPORT jint JNICALL
Java_com_night_backgroundchange_MainActivity_getNativeLevel(JNIEnv* env, jobject thiz) {
    return g_currentLevel;
}

JNIEXPORT void JNICALL
Java_com_night_backgroundchange_MainActivity_resetNativeGame(JNIEnv* env, jobject thiz) {
    g_playerLives = 3;
    g_playerX = 0.0f; // Center default resets
    g_playerY = 0.0f;
    g_targetX = 0.0f;
    g_targetY = 0.0f;
}

JNIEXPORT void JNICALL
Java_com_night_backgroundchange_MainActivity_updateNativeGame(JNIEnv* env, jobject thiz) {
    if (g_gameState == 2 && g_playerLives > 0) {
        // Smooth interpolation calculations for tracking user touch targets
        if (g_targetX != 0.0f) {
            g_playerX += (g_targetX - g_playerX) * 0.15f;
            g_playerY += (g_targetY - g_playerY) * 0.15f;
        }

        // Loop ticker simulating tactical maze hazard traps
        g_tickCounter++;
        if (g_tickCounter % 140 == 0) {
            g_playerLives--; // Simulate collision/wrong path selection
        }
    }
}

JNIEXPORT void JNICALL
Java_com_night_backgroundchange_MainActivity_handleNativeTouch(JNIEnv* env, jobject thiz, jfloat x, jfloat y) {
    if (g_gameState == 2) {
        g_targetX = x;
        g_targetY = y;
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
