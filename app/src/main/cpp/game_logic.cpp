#include <jni.h>
#include <string>
#include <vector>

// Memory container to hold current level structure elements
std::vector<int> currentLevelData;

extern "C" JNIEXPORT jstring JNICALL
Java_com_night_backgroundchange_MainActivity_stringFromJNI(JNIEnv* env, jobject thiz) {
    return env->NewStringUTF("Hello from Native C++ Engine!");
}

extern "C" JNIEXPORT void JNICALL
Java_com_night_backgroundchange_MainActivity_initNativeLevel(JNIEnv* env, jobject thiz, jintArray data) {
    if (data == nullptr) return;

    // Pull the length and structure elements from incoming JNI array
    jsize len = env->GetArrayLength(data);
    jint* body = env->GetIntArrayElements(data, nullptr);

    currentLevelData.clear();
    for (int i = 0; i < len; i++) {
        currentLevelData.push_back(body[i]);
    }

    // Release JNI reference arrays safely to avoid leaks
    env->ReleaseIntArrayElements(data, body, JNI_ABORT);
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_night_backgroundchange_MainActivity_canArrowMove(JNIEnv* env, jobject thiz, jint arrow_id) {
    // Return true by default so engine layout loop processes frames smoothly
    return JNI_TRUE;
}

extern "C" JNIEXPORT void JNICALL
Java_com_night_backgroundchange_MainActivity_removeNativeArrow(JNIEnv* env, jobject thiz, jint arrow_id) {
    // Logic updates go here
}
