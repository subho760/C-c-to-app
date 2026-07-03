#include <jni.h>
#include <android/log.h>
#include <GLES2/gl2.h>
#include <chrono>
#include <vector>

#define LOG_TAG "ArrowsEngine"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

// --- 1. GAME STATES ---
enum GameState {
    STATE_LOADING,    // Boot-up / Assets
    STATE_MAIN_MENU,  // Level display & Play button
    STATE_PLAYING,    // Active gameplay
    STATE_GAME_OVER   // Out of lives / Ad prompt
};

// --- 2. COLOR PALETTE (RGB Floats) ---
struct Theme {
    float background[4] = {0.0f, 0.0f, 0.0f, 1.0f};    // #000000
    float neonPath[4]   = {0.0f, 0.95f, 1.0f, 1.0f};  // Neon Cyan (#00F3FF)
    float dimmedPath[4] = {0.15f, 0.15f, 0.15f, 1.0f}; // Background maze decoration
    float textWhite[4]  = {0.9f, 0.9f, 0.9f, 1.0f};    // Level indicator
};

// --- 3. GLOBAL ENGINE VARIABLES ---
GameState currentGameState = STATE_LOADING;
Theme gameTheme;
float loadingProgress = 0.0f;
int currentLevel = 92; // Default starting level per video
int screenWidth = 0;
int screenHeight = 0;

// Timer for animations/loading
auto lastFrameTime = std::chrono::high_resolution_clock::now();

// --- 4. CORE ENGINE LOGIC ---

/**
 * Updates the game logic based on the current state.
 */
void updateEngine() {
    auto currentTime = std::chrono::high_resolution_clock::now();
    float deltaTime = std::chrono::duration<float>(currentTime - lastFrameTime).count();
    lastFrameTime = currentTime;

    switch (currentGameState) {
        case STATE_LOADING:
            // Simulate asset loading or initialization
            loadingProgress += deltaTime * 0.5f; // Fills in 2 seconds
            if (loadingProgress >= 1.0f) {
                currentGameState = STATE_MAIN_MENU;
                LOGI("Transitioning to MAIN_MENU");
            }
            break;

        case STATE_MAIN_MENU:
            // Logic for pulsing the "Play" button or background arrows
            break;

        case STATE_PLAYING:
            // Gameplay path-finding logic goes here
            break;

        case STATE_GAME_OVER:
            break;
    }
}

/**
 * Handles the rendering loop calls.
 */
void renderEngine() {
    // Clear to minimalist black background
    glClearColor(gameTheme.background[0], gameTheme.background[1], 
                 gameTheme.background[2], gameTheme.background[3]);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (currentGameState == STATE_LOADING) {
        // Here you would render a simple progress bar or "Loading..." text
        // using the neonPath color.
    } 
    else if (currentGameState == STATE_MAIN_MENU) {
        // 1. Render the background arrow maze using dimmedPath color
        // 2. Render the "Level X" text
        // 3. Render the "Play" button asset
    }
}

// --- 5. JNI BINDINGS ---

extern "C" {

JNIEXPORT void JNICALL
Java_com_night_backgroundchange_MainActivity_nativeOnSurfaceCreated(JNIEnv* env, jobject thiz) {
    lastFrameTime = std::chrono::high_resolution_clock::now();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

JNIEXPORT void JNICALL
Java_com_night_backgroundchange_MainActivity_nativeOnSurfaceChanged(JNIEnv* env, jobject thiz, jint width, jint height) {
    screenWidth = width;
    screenHeight = height;
    glViewport(0, 0, width, height);
}

JNIEXPORT void JNICALL
Java_com_night_backgroundchange_MainActivity_nativeStep(JNIEnv* env, jobject thiz) {
    updateEngine();
    renderEngine();
}

// Transition from Menu to Game
JNIEXPORT void JNICALL
Java_com_night_backgroundchange_MainActivity_nativeOnPlayClicked(JNIEnv* env, jobject thiz) {
    if (currentGameState == STATE_MAIN_MENU) {
        currentGameState = STATE_PLAYING;
        LOGI("Game Started at Level %d", currentLevel);
    }
}

} // extern "C"
