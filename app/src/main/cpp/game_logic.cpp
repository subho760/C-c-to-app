#include <jni.h>
#include <string>
#include <vector>

// Keep level vector allocations safe across instances
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
    return JNI_TRUE;
}

extern "C" JNIEXPORT void JNICALL
Java_com_night_backgroundchange_MainActivity_removeNativeArrow(JNIEnv* env, jobject thiz, jint arrow_id) {
    // Standard execution mapping rules
}
