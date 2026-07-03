#include <jni.h>
#include <android/log.h>
#include <GLES2/gl2.h>
#include <vector>
#include <string>
#include <chrono>

#define LOG_TAG "ArrowsEngine"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

// --- Game Constants & State ---
enum GameState { LOADING, MENU, PLAYING, GAMEOVER };
GameState currentState = LOADING;

float bgColor[3] = {0.0f, 0.0f, 0.0f};       // Minimalist #000000
float neonCyan[3] = {0.0f, 0.95f, 1.0f};     // Neon Cyan #00F3FF
float pathDim[3] = {0.1f, 0.1f, 0.1f};       // Dimmed arrows

int playerLives = 3;
int currentLevel = 92;
float loadingProgress = 0.0f;

// JNI Helpers
JavaVM* g_JavaVM = nullptr;
jobject g_MainActivityObj = nullptr;

extern "C" {

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    g_JavaVM = vm;
    return JNI_VERSION_1_6;
}

// --- Lifecycle Methods ---

JNIEXPORT void JNICALL
Java_com_night_backgroundchange_MainActivity_nativeOnSurfaceCreated(JNIEnv* env, jobject thiz) {
    glClearColor(bgColor[0], bgColor[1], bgColor[2], 1.0f);
    // Initialize Shaders and Buffers for Arrow Geometry here
}

JNIEXPORT void JNICALL
Java_com_night_backgroundchange_MainActivity_nativeOnSurfaceChanged(JNIEnv* env, jobject thiz, jint w, jint h) {
    glViewport(0, 0, w, h);
}

JNIEXPORT void JNICALL
Java_com_night_backgroundchange_MainActivity_nativeStep(JNIEnv* env, jobject thiz) {
    glClear(GL_COLOR_BUFFER_BIT);

    switch (currentState) {
        case LOADING:
            loadingProgress += 0.01f;
            if (loadingProgress >= 1.0f) {
                currentState = MENU;
                // Callback to Java to hide loading UI
                jclass clazz = env->GetObjectClass(thiz);
                jmethodID method = env->GetMethodID(clazz, "onGameLoadingComplete", "()V");
                env->CallVoidMethod(thiz, method);
            }
            break;

        case PLAYING:
            // Logic to move the pulse along the arrow paths
            // logic_check_collisions();
            
            // Example Game Over Trigger (when lives hit 0)
            if (playerLives <= 0) {
                currentState = GAMEOVER;
                jclass clazz = env->GetObjectClass(thiz);
                jmethodID method = env->GetMethodID(clazz, "triggerRewardedAd", "()V");
                env->CallVoidMethod(thiz, method);
            }
            
            // Draw Arrow Maze with Neon Cyan paths
            // render_arrow_maze(neonCyan);
            break;
            
        case MENU:
        case GAMEOVER:
            // Idle rendering
            break;
    }
}

// --- Input Handling ---

JNIEXPORT void JNICALL
Java_com_night_backgroundchange_MainActivity_nativeOnTouch(JNIEnv* env, jobject thiz, jfloat x, jfloat y) {
    if (currentState == PLAYING) {
        LOGI("Touch registered at %f, %f. Calculating path...", x, y);
        // Path logic: Find nearest arrow node and start movement
    }
}

JNIEXPORT void JNICALL
Java_com_night_backgroundchange_MainActivity_nativeOnPlayClicked(JNIEnv* env, jobject thiz) {
    currentState = PLAYING;
    playerLives = 3;
    LOGI("Game Started - Level %d", currentLevel);
}

// --- Ad Success Callback ---

JNIEXPORT void JNICALL
Java_com_night_backgroundchange_MainActivity_nativeGrantLives(JNIEnv* env, jobject thiz) {
    playerLives = 3;
    currentState = PLAYING;
    LOGI("Lives granted via Ad. Continuing game...");
}

JNIEXPORT void JNICALL
Java_com_night_backgroundchange_MainActivity_nativeInitAssetManager(JNIEnv* env, jobject thiz, jobject assetManager) {
    // Store reference to asset manager for loading level path data from JSON/Binary
}

} // extern "C"
