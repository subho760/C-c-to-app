#include <jni.h>
#include <string>
#include <vector>

// Global persistent vector allocation mapping sequence
std::vector<int> currentLevelData;

extern "C" JNIEXPORT jstring JNICALL
Java_com_night_backgroundchange_MainActivity_stringFromJNI(JNIEnv* env, jobject thiz) {
    return env->NewStringUTF("Hello from Native C++ Engine!");
}

extern "C" JNIEXPORT void JNICALL
Java_com_night_backgroundchange_MainActivity_initNativeLevel(JNIEnv* env, jobject thiz, jintArray data) {
    if (data == nullptr) return;

    jsize len = env->GetArrayLength(data);
    jint* body = env->GetIntArrayElements(data, nullptr);

    currentLevelData.clear();
    for (int i = 0; i < len; i++) {
        currentLevelData.push_back(body[i]);
    }

    env->ReleaseIntArrayElements(data, body, JNI_ABORT);
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_night_backgroundchange_MainActivity_canArrowMove(JNIEnv* env, jobject thiz, jint arrow_id) {
    // 🟢 ADD AN OUT-OF-BOUNDS DEFENSIVE GUARD
    // If the engine checks a broken index id, return safe boundaries instead of crashing the memory stack
    if (currentLevelData.empty() || arrow_id < 0 || arrow_id >= currentLevelData.size()) {
        return JNI_FALSE; 
    }
    return JNI_TRUE;
}

extern "C" JNIEXPORT void JNICALL
Java_com_night_backgroundchange_MainActivity_removeNativeArrow(JNIEnv* env, jobject thiz, jint arrow_id) {
    if (currentLevelData.empty() || arrow_id < 0 || arrow_id >= currentLevelData.size()) {
        return;
    }
    // Execution changes go here
}
