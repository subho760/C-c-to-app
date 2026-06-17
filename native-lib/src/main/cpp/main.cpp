
#include <android/log.h>
#include <android_native_app_glue.h>
#include "my_logic.h"

#define LOG_TAG "NativeApp"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

// The ultimate engine entry point for your app lifecycle
void android_main(struct android_app* state) {
    LOGI("Pure C++ Runner Initialized successfully!");
    
    int result = calculateSomething(10, 20);
    LOGI("Testing logic header: 10 + 20 = %d", result);

    while (true) {
        int events;
        struct android_poll_source* source;

        if (ALooper_pollAll(-1, nullptr, &events, (void**)&source) >= 0) {
            if (source != nullptr) {
                source->process(state, source);
            }
        }

        if (state->destroyRequested != 0) {
            LOGI("Terminating C++ application execution.");
            return;
        }
    }
}
