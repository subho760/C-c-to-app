#include "game_structures.h"

GameMenuStructure gameUI;

void renderBmp(JNIEnv* env, jobject canvas, jobject bitmap, float leftX, float topY, float forcedWidth, jobject customPaint) {
    if (!canvas || !bitmap || !gameUI.midSave) return;

    int nativeBmpW = 100, nativeBmpH = 100;
    if (gameUI.midGetWidth) nativeBmpW = env->CallIntMethod(bitmap, gameUI.midGetWidth);
    if (gameUI.midGetHeight) nativeBmpH = env->CallIntMethod(bitmap, gameUI.midGetHeight);

    float scaleFactor = forcedWidth / (float)nativeBmpW;

    env->CallIntMethod(canvas, gameUI.midSave);
    env->CallVoidMethod(canvas, gameUI.midTranslate, (jfloat)leftX, (jfloat)topY);
    env->CallVoidMethod(canvas, gameUI.midScale, (jfloat)scaleFactor, (jfloat)scaleFactor, 0.0f, 0.0f);
    env->CallVoidMethod(canvas, gameUI.midDrawBitmap, bitmap, 0.0f, 0.0f, customPaint);
    env->CallVoidMethod(canvas, gameUI.midRestore);
}

void drawRealShadowRoundRect(JNIEnv* env, jobject canvas, float left, float top, float right, float bottom, float rx, float ry) {
    if (!canvas || !gameUI.midDrawRoundRect || !gameUI.paintShapeReference) return;
    jclass paintCls = env->GetObjectClass(gameUI.paintShapeReference);
    jmethodID setColor = env->GetMethodID(paintCls, "setColor", "(I)V");

    env->CallVoidMethod(gameUI.paintShapeReference, setColor, gameUI.isCurrentlyDark ? 0x55000000 : 0x22000000);
    env->CallVoidMethod(canvas, gameUI.midDrawRoundRect, left + 4.0f, top + 8.0f, right + 4.0f, bottom + 8.0f, rx, ry, gameUI.paintShapeReference);
}

void drawRoundRectNative(JNIEnv* env, jobject canvas, float left, float top, float right, float bottom, float rx, float ry, int colorHex) {
    if (!canvas || !gameUI.midDrawRoundRect || !gameUI.paintShapeReference) return;
    jclass paintCls = env->GetObjectClass(gameUI.paintShapeReference);
    jmethodID setColor = env->GetMethodID(paintCls, "setColor", "(I)V");
    env->CallVoidMethod(gameUI.paintShapeReference, setColor, colorHex);
    env->CallVoidMethod(canvas, gameUI.midDrawRoundRect, left, top, right, bottom, rx, ry, gameUI.paintShapeReference);
}

int getNextUnlockableLevel() {
    for (int i = 0; i < 50; i++) {
        if (!gameUI.levelsUnlocked[i]) return i;
    }
    return -1;
}

void setPaintFontWeight(JNIEnv* env, jobject paintRef, bool isBold) {
    if (!paintRef) return;
    jclass paintCls = env->GetObjectClass(paintRef);
    jmethodID setFakeBold = env->GetMethodID(paintCls, "setFakeBoldText", "(Z)V");
    if (setFakeBold) {
        env->CallVoidMethod(paintRef, setFakeBold, isBold ? JNI_TRUE : JNI_FALSE);
    }
}

jobject getTintPaint(JNIEnv* env, jobject obj, int colorHex) {
    jclass actCls = env->GetObjectClass(obj);
    jmethodID getTintMid = env->GetMethodID(actCls, "getTintedPaint", "(I)Landroid/graphics/Paint;");
    if (getTintMid) return env->CallObjectMethod(obj, getTintMid, colorHex);
    return nullptr;
}

extern "C" {

JNIEXPORT void JNICALL
Java_com_night_backgroundchange_MainActivity_initNativeEngine(JNIEnv* env, jobject obj, jboolean systemDark) {
    gameUI.engineInitialized = false;

    if (gameUI.activeTheme == THEME_SYSTEM) {
        gameUI.isCurrentlyDark = systemDark;
    } else {
        gameUI.isCurrentlyDark = (gameUI.activeTheme == THEME_DARK);
    }

    jclass localCanvas = env->FindClass("android/graphics/Canvas");
    if (localCanvas) gameUI.canvasClass = (jclass)env->NewGlobalRef(localCanvas);
    
    jclass localPaint = env->FindClass("android/graphics/Paint");
    if (localPaint) gameUI.paintClass = (jclass)env->NewGlobalRef(localPaint);

    jclass localBitmap = env->FindClass("android/graphics/Bitmap");
    if (localBitmap) gameUI.bitmapClass = (jclass)env->NewGlobalRef(localBitmap);

    if (gameUI.canvasClass) {
        gameUI.midDrawColor = env->GetMethodID(gameUI.canvasClass, "drawColor", "(I)V");
        gameUI.midSave = env->GetMethodID(gameUI.canvasClass, "save", "()I");
        gameUI.midTranslate = env->GetMethodID(gameUI.canvasClass, "translate", "(FF)V");
        gameUI.midScale = env->GetMethodID(gameUI.canvasClass, "scale", "(FFFF)V");
        gameUI.midDrawBitmap = env->GetMethodID(gameUI.canvasClass, "drawBitmap", "(Landroid/graphics/Bitmap;FFLandroid/graphics/Paint;)V");
        gameUI.midRestore = env->GetMethodID(gameUI.canvasClass, "restore", "()V");
        gameUI.midDrawText = env->GetMethodID(gameUI.canvasClass, "drawText", "(Ljava/lang/String;FFLandroid/graphics/Paint;)V");
        gameUI.midDrawRoundRect = env->GetMethodID(gameUI.canvasClass, "drawRoundRect", "(FFFFFFLandroid/graphics/Paint;)V");
        gameUI.midDrawLine = env->GetMethodID(gameUI.canvasClass, "drawLine", "(FFFFLandroid/graphics/Paint;)V");
        gameUI.midDrawCircle = env->GetMethodID(gameUI.canvasClass, "drawCircle", "(FFFLandroid/graphics/Paint;)V");
    }

    if (gameUI.bitmapClass) {
        gameUI.midGetWidth = env->GetMethodID(gameUI.bitmapClass, "getWidth", "()I");
        gameUI.midGetHeight = env->GetMethodID(gameUI.bitmapClass, "getHeight", "()I");
    }

    if (gameUI.paintClass) {
        jmethodID paintInit = env->GetMethodID(gameUI.paintClass, "<init>", "()V");
        jobject textPaint = env->NewObject(gameUI.paintClass, paintInit);
        jobject shapePaint = env->NewObject(gameUI.paintClass, paintInit);
        
        jmethodID setAntiAlias = env->GetMethodID(gameUI.paintClass, "setAntiAlias", "(Z)V");
        env->CallVoidMethod(textPaint, setAntiAlias, JNI_TRUE);
        env->CallVoidMethod(shapePaint, setAntiAlias, JNI_TRUE);
        
        gameUI.paintTextReference = env->NewGlobalRef(textPaint);
        gameUI.paintShapeReference = env->NewGlobalRef(shapePaint);

        env->DeleteLocalRef(textPaint);
        env->DeleteLocalRef(shapePaint);
    }

    gameUI.engineInitialized = true;
}

JNIEXPORT void JNICALL
Java_com_night_backgroundchange_MainActivity_nativePushAsset(JNIEnv* env, jobject obj, jint index, jobject bmp) {
    if (index >= 0 && index < ASSET_COUNT && bmp) {
        gameUI.assetBitmaps[index] = env->NewGlobalRef(bmp);
    }
}

JNIEXPORT void JNICALL
Java_com_night_backgroundchange_MainActivity_nativeOnResize(JNIEnv* env, jobject obj, jint w, jint h) {
    gameUI.screenWidth = w;
    gameUI.screenHeight = h;
}

JNIEXPORT void JNICALL
Java_com_night_backgroundchange_MainActivity_nativeApplyScroll(JNIEnv* env, jobject obj, jfloat deltaY) {
    if (gameUI.currentState == STATE_LEVELS) {
        gameUI.levelScrollOffset += deltaY;
        if (gameUI.levelScrollOffset > 0.0f) gameUI.levelScrollOffset = 0.0f;
        if (gameUI.levelScrollOffset < -gameUI.maxScrollExtent) gameUI.levelScrollOffset = -gameUI.maxScrollExtent;
    }
}

JNIEXPORT void JNICALL
Java_com_night_backgroundchange_MainActivity_nativeOnTouch(JNIEnv* env, jobject obj, jfloat x, jfloat y) {
    if (!gameUI.engineInitialized) return;

    // Fix #1: Process close hits systematically on any visible pop-ups
    checkGlobalClosePopupDismiss(x, y);

    bool blockingPopup = (gameUI.isHintPopupActive || gameUI.isThemePopupActive || gameUI.isRatingPopupActive || gameUI.isPausePopupActive);

    for (const auto& btn : gameUI.UIButtons) {
        if (x >= btn.x && x <= btn.x + btn.w && y >= btn.y && y <= btn.y + btn.h) {
            
            if (btn.actionCode == 9999) { 
                gameUI.isHintPopupActive = false;
                gameUI.isThemePopupActive = false;
                gameUI.isRatingPopupActive = false;
                gameUI.isPausePopupActive = false;
                return;
            }

            // Fix #3: Seamless functional logic when toggling active theme layers
            if (gameUI.isThemePopupActive && btn.actionCode >= 7700 && btn.actionCode <= 7702) {
                gameUI.activeTheme = (ThemeMode)(btn.actionCode - 7700);
                gameUI.isThemePopupActive = false;
                jclass actCls = env->GetObjectClass(obj);
                jmethodID refreshMid = env->GetMethodID(actCls, "triggerThemeRebuild", "()V");
                if (refreshMid) env->CallVoidMethod(obj, refreshMid);
                return;
            }

            if (gameUI.isRatingPopupActive) {
                if (btn.actionCode >= 7800 && btn.actionCode <= 7804) {
                    gameUI.selectedRatingStars = (btn.actionCode - 7800) + 1;
                } else if (btn.actionCode == 7899) {
                    gameUI.isRatingPopupActive = false;
                }
                return;
            }

            if (gameUI.isHintPopupActive && btn.actionCode == 8801) {
                gameUI.isHintPopupActive = false;
                return;
            }

            if (gameUI.isPausePopupActive) {
                if (btn.actionCode == 5501) gameUI.isPausePopupActive = false;
                else if (btn.actionCode == 5502) gameUI.isPausePopupActive = false;
                else if (btn.actionCode == 5503) gameUI.audioEnabled = !gameUI.audioEnabled;
                else if (btn.actionCode == 5504) {
                    gameUI.isPausePopupActive = false;
                    gameUI.currentState = STATE_HOME;
                }
                return;
            }

            if (blockingPopup) return; 

            if (btn.actionCode >= 3000 && btn.actionCode <= 3049) {
                int lvlIdx = btn.levelValue;
                int nextIdx = getNextUnlockableLevel();
                if (gameUI.levelsUnlocked[lvlIdx] || lvlIdx == nextIdx) {
                    gameUI.levelsUnlocked[lvlIdx] = true;
                    gameUI.currentPlayingLevel = lvlIdx + 1;
                    gameUI.currentState = STATE_GAMEPLAY;
                }
                break;
            }

            switch (btn.actionCode) {
                case 9001: gameUI.currentState = STATE_HOME; break;
                case 9002: gameUI.currentState = STATE_LEVELS; break;
                case 9003: gameUI.currentState = STATE_SETTINGS; break;
                case 2001: gameUI.currentState = STATE_GAMEPLAY; break;
                case 2010: { 
                    jclass actCls = env->GetObjectClass(obj);
                    jmethodID purchaseMid = env->GetMethodID(actCls, "dispatchRemoveAdsPurchase", "()V");
                    if (purchaseMid) env->CallVoidMethod(obj, purchaseMid);
                    break;
                }
                case 3500:
                    for(int i = 0; i < 50; i++) gameUI.levelsUnlocked[i] = true;
                    gameUI.currentState = STATE_LEVELS;
                    break;
                case 4001:
                    if (gameUI.currentPlayingLevel < 50) {
                        gameUI.levelsUnlocked[gameUI.currentPlayingLevel] = true;
                        gameUI.currentPlayingLevel++;
                    }
                    gameUI.currentState = STATE_LEVELS;
                    break;
                case 4002: gameUI.isPausePopupActive = true; break;
                case 4003: gameUI.isHintPopupActive = true; break;
                case 6501: gameUI.isThemePopupActive = true; break;
                case 6504: 
                    gameUI.selectedRatingStars = 0;
                    gameUI.isRatingPopupActive = true; 
                    break;
                case 6502: {
                    jclass actCls = env->GetObjectClass(obj);
                    jmethodID midIntent = env->GetMethodID(actCls, "dispatchPrivacyPolicyIntent", "()V");
                    if (midIntent) env->CallVoidMethod(obj, midIntent);
                    break;
                }
                case 6503: {
                    jclass actCls = env->GetObjectClass(obj);
                    jmethodID midShare = env->GetMethodID(actCls, "dispatchShareIntent", "()V");
                    if (midShare) env->CallVoidMethod(obj, midShare);
                    break;
                }
            }
            break;
        }
    }
}

} // extern "C"
