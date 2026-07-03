#include <jni.h>
#include <vector>
#include <cmath>

// --- Game Logic Constants ---
enum GameState { STATE_LOADING, STATE_MENU, STATE_PLAYING, STATE_GAMEOVER };

struct Arrow {
    float x, y;    // Normalized (0.0 to 1.0)
    int direction; // 0:U, 1:R, 2:D, 3:L
    bool isPath;   // Part of the level solution
    bool isActive; // Currently lit up
};

// --- Global State ---
GameState g_state = STATE_LOADING;
std::vector<Arrow> g_levelData;
int g_lives = 3;
int g_level = 92;
float g_loadTimer = 0.0f;

// Create Level 92 Layout (Simplified simulation of video)
void buildLevel92() {
    g_levelData.clear();
    // Create a grid of arrows
    for (int i = 0; i < 5; ++i) {
        for (int j = 0; j < 5; ++j) {
            bool solve = (i == j); // Dummy diagonal logic for example
            g_levelData.push_back({0.2f + (j * 0.15f), 0.3f + (i * 0.1f), (i + j) % 4, solve, false});
        }
    }
}

extern "C" {

JNIEXPORT void JNICALL
Java_com_night_backgroundchange_MainActivity_nativeInit(JNIEnv* env, jobject thiz) {
    g_state = STATE_LOADING;
    g_loadTimer = 0.0f;
    g_lives = 3;
    buildLevel92();
}

JNIEXPORT jint JNICALL
Java_com_night_backgroundchange_MainActivity_nativeUpdate(JNIEnv* env, jobject thiz, jfloat dt) {
    if (g_state == STATE_LOADING) {
        g_loadTimer += dt;
        if (g_loadTimer > 1.5f) g_state = STATE_MENU;
    }
    return (jint)g_state;
}

JNIEXPORT jint JNICALL
Java_com_night_backgroundchange_MainActivity_nativeGetArrowCount(JNIEnv* env, jobject thiz) {
    return (jint)g_levelData.size();
}

// Returns: [x, y, dir, isPath, isActive]
JNIEXPORT jfloatArray JNICALL
Java_com_night_backgroundchange_MainActivity_nativeGetArrowData(JNIEnv* env, jobject thiz, jint index) {
    Arrow& a = g_levelData[index];
    jfloatArray result = env->NewFloatArray(5);
    float vals[5] = {a.x, a.y, (float)a.direction, a.isPath ? 1.0f : 0.0f, a.isActive ? 1.0f : 0.0f};
    env->SetFloatArrayRegion(result, 0, 5, vals);
    return result;
}

JNIEXPORT void JNICALL
Java_com_night_backgroundchange_MainActivity_nativeHandleTouch(JNIEnv* env, jobject thiz, jfloat tx, jfloat ty) {
    if (g_state != STATE_PLAYING) return;

    for (auto& a : g_levelData) {
        float dx = a.x - tx;
        float dy = a.y - ty;
        if (std::sqrt(dx*dx + dy*dy) < 0.05f) {
            if (a.isPath) {
                a.isActive = true;
            } else {
                g_lives--;
                if (g_lives <= 0) {
                    g_state = STATE_GAMEOVER;
                    // Trigger Ad Callback in Java
                    jclass cls = env->GetObjectClass(thiz);
                    jmethodID mid = env->GetMethodID(cls, "onTriggerGameOverAd", "()V");
                    env->CallVoidMethod(thiz, mid);
                }
            }
            break;
        }
    }
}

JNIEXPORT void JNICALL
Java_com_night_backgroundchange_MainActivity_nativeOnPlay(JNIEnv* env, jobject thiz) {
    g_state = STATE_PLAYING;
}

JNIEXPORT void JNICALL
Java_com_night_backgroundchange_MainActivity_nativeAddLives(JNIEnv* env, jobject thiz) {
    g_lives = 3;
    g_state = STATE_PLAYING;
}

} // extern "C"
