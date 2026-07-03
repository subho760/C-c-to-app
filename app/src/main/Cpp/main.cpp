#include <jni.h>
#include <vector>
#include <string>

// --- Game States ---
enum GameState { STATE_LOADING, STATE_MENU, STATE_PLAYING, STATE_GAMEOVER };
GameState currentState = STATE_LOADING;

// --- Data Structures ---
struct Arrow {
    float x1, y1, x2, y2; // Start and end points
    int direction;        // 0: Up, 1: Right, 2: Down, 3: Left
    bool isActive;        // Is it part of the current neon path?
};

// Global Game Data
std::vector<Arrow> levelArrows;
int playerLives = 3;
int currentLevel = 92;
float loadingProgress = 0.0f;

// Initialize a mock level (Level 92) based on video patterns
void loadLevelData() {
    levelArrows.clear();
    // Simplified example: Adding a few arrows to represent the maze
    // In a real build, you'd load these from a JSON or array
    levelArrows.push_back({100, 500, 100, 300, 0, false}); // Up
    levelArrows.push_back({100, 300, 400, 300, 1, false}); // Right
    levelArrows.push_back({400, 300, 400, 600, 2, false}); // Down
}

extern "C" {

// Initialize Logic
JNIEXPORT void JNICALL
Java_com_night_backgroundchange_MainActivity_nativeInit(JNIEnv* env, jobject thiz) {
    currentState = STATE_LOADING;
    loadingProgress = 0.0f;
    loadLevelData();
}

// Logic Update (Called every frame from Java)
JNIEXPORT jint JNICALL
Java_com_night_backgroundchange_MainActivity_nativeUpdate(JNIEnv* env, jobject thiz, jfloat delta) {
    if (currentState == STATE_LOADING) {
        loadingProgress += delta;
        if (loadingProgress >= 2.0f) currentState = STATE_MENU;
    }
    return (jint)currentState;
}

// Get number of arrows to draw
JNIEXPORT jint JNICALL
Java_com_night_backgroundchange_MainActivity_nativeGetArrowCount(JNIEnv* env, jobject thiz) {
    return levelArrows.size();
}

// Get specific arrow data for Java Canvas
JNIEXPORT jfloatArray JNICALL
Java_com_night_backgroundchange_MainActivity_nativeGetArrowData(JNIEnv* env, jobject thiz, jint index) {
    if (index >= levelArrows.size()) return nullptr;
    
    Arrow& a = levelArrows[index];
    jfloatArray result = env->NewFloatArray(6);
    float fill[6] = { a.x1, a.y1, a.x2, a.y2, (float)a.direction, (float)(a.isActive ? 1 : 0) };
    env->SetFloatArrayRegion(result, 0, 6, fill);
    return result;
}

JNIEXPORT void JNICALL
Java_com_night_backgroundchange_MainActivity_nativeOnPlayClicked(JNIEnv* env, jobject thiz) {
    currentState = STATE_PLAYING;
    playerLives = 3;
}

// Process Touch Logic
JNIEXPORT void JNICALL
Java_com_night_backgroundchange_MainActivity_nativeTouch(JNIEnv* env, jobject thiz, jfloat tx, jfloat ty) {
    if (currentState != STATE_PLAYING) return;

    // Simple collision: if touch is near an arrow, activate it
    for (auto& a : levelArrows) {
        if (std::abs(tx - a.x1) < 50 && std::abs(ty - a.y1) < 50) {
            a.isActive = true;
        }
    }
}

} // extern "C"
