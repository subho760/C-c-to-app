#include <jni.h>
#include <string>
#include <vector>

// Global array memory container to hold your game's current map matrix array data
std::vector<int> currentLevelData;

extern "C" JNIEXPORT jstring JNICALL
Java_com_night_backgroundchange_MainActivity_stringFromJNI(JNIEnv* env, jobject thiz) {
    return env->NewStringUTF("Hello from Native C++ Engine!");
}

extern "C" JNIEXPORT void JNICALL
Java_com_night_backgroundchange_MainActivity_initNativeLevel(JNIEnv* env, jobject thiz, jintArray data) {
    if (data == nullptr) return;

    // Extract length of incoming level array data
    jsize len = env->GetArrayLength(data);
    jint* body = env->GetIntArrayElements(data, nullptr);

    currentLevelData.clear();
    for (int i = 0; i < len; i++) {
        currentLevelData.push_back(body[i]);
    }

    // Release JNI reference safely to avoid memory leak states
    env->ReleaseIntArrayElements(data, body, JNI_ABORT);
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_night_backgroundchange_MainActivity_canArrowMove(JNIEnv* env, jobject thiz, jint arrow_id) {
    // Game loop layout validator logic can check against currentLevelData vector elements here
    return JNI_TRUE;
}

extern "C" JNIEXPORT void JNICALL
Java_com_night_backgroundchange_MainActivity_removeNativeArrow(JNIEnv* env, jobject thiz, jint arrow_id) {
    // Level map modification tracker handler
}
