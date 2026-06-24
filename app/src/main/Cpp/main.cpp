#include <jni.h>
#include <string>
#include <cstdlib>
#include <ctime>

extern "C" JNIEXPORT jstring JNICALL
Java_com_night_backgroundchange_MainActivity_getRandomColorHex(JNIEnv* env, jobject thiz) {
    // String array containing hex color values
    const char* colors[] = {
        "#FF5733", "#33FF57", "#3357FF", "#F3FF33", 
        "#FF33F3", "#33FFF0", "#000000", "#7D3C98",
        "#229954", "#D35400", "#2E4053", "#1ABC9C"
    };
    
    // Seed and pick a random color index
    static bool seeded = false;
    if (!seeded) {
        srand(time(nullptr));
        seeded = true;
    }
    int randomIndex = rand() % 12;
    
    return env->NewStringUTF(colors[randomIndex]);
}
