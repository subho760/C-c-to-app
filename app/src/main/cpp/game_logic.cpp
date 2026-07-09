#include <jni.h>
#include <vector>

enum Direction { UP = 0, RIGHT = 1, DOWN = 2, LEFT = 3 };

struct Arrow {
    int id;
    int x; 
    int y; 
    int dir;
    bool isMoving;
};

std::vector<Arrow> currentLevelArrows;

extern "C" JNIEXPORT void JNICALL
Java_com_night_backgroundchange_MainActivity_initNativeLevel(JNIEnv *env, jobject thiz, jintArray data) {
    currentLevelArrows.clear();
    jsize len = env->GetArrayLength(data);
    jint *body = env->GetIntArrayElements(data, 0);
    
    for (int i = 0; i < len; i += 4) {
        Arrow a;
        a.id = body[i];
        a.x = body[i+1];
        a.y = body[i+2];
        a.dir = body[i+3];
        a.isMoving = false;
        currentLevelArrows.push_back(a);
    }
    env->ReleaseIntArrayElements(data, body, 0);
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_night_backgroundchange_MainActivity_canArrowMove(JNIEnv *env, jobject thiz, jint arrowId) {
    Arrow* target = nullptr;
    for (auto &a : currentLevelArrows) {
        if (a.id == arrowId) {
            target = &a;
            break;
        }
    }
    if (!target) return JNI_FALSE;

    for (auto &other : currentLevelArrows) {
        if (other.id == arrowId || other.isMoving) continue;

        if (target->dir == UP) {
            if (other.x == target->x && other.y < target->y) return JNI_FALSE;
        } else if (target->dir == RIGHT) {
            if (other.y == target->y && other.x > target->x) return JNI_FALSE;
        } else if (target->dir == DOWN) {
            if (other.x == target->x && other.y > target->y) return JNI_FALSE;
        } else if (target->dir == LEFT) {
            if (other.y == target->y && other.x < target->x) return JNI_FALSE;
        }
    }
    return JNI_TRUE;
}

extern "C" JNIEXPORT void JNICALL
Java_com_night_backgroundchange_MainActivity_removeNativeArrow(JNIEnv *env, jobject thiz, jint arrowId) {
    for (auto it = currentLevelArrows.begin(); it != currentLevelArrows.end(); ++it) {
        if (it->id == arrowId) {
            currentLevelArrows.erase(it);
            break;
        }
    }
}
