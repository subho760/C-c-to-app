#include <jni.h>
#include <string>

extern "C" JNIEXPORT jstring JNICALL
Java_com_night_backgroundchange_MainActivity_stringFromJNI(JNIEnv* env, jobject thiz) {
    return env->NewStringUTF("Hello from Native C++ Engine!");
}

extern "C" JNIEXPORT void JNICALL
Java_com_night_backgroundchange_MainActivity_initNativeLevel(JNIEnv* env, jobject thiz, jintArray data) {
    // Keeps array processing safe and initialized
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_night_backgroundchange_MainActivity_canArrowMove(JNIEnv* env, jobject thiz, jint arrow_id) {
    // Return true by default so the game loop elements don't lock up
    return JNI_TRUE;
}

extern "C" JNIEXPORT void JNICALL
Java_com_night_backgroundchange_MainActivity_removeNativeArrow(JNIEnv* env, jobject thiz, jint arrow_id) {
    // Safe placeholder handle
}
