#include <android/log.h>
#include <android_native_app_glue.h>

#define LOG_TAG "NativeGameLoop"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

// Track our background state
bool isBackgroundChanged = false;

// Function to handle your touch/click inputs
static int32_t handle_input(struct android_app* app, AInputEvent* event) {
    // Check if the event is a screen touch (motion event)
    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
        int32_t action = AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;
        
        // Triggered exactly when you press your finger down on the phone screen
        if (action == AMOTION_EVENT_ACTION_DOWN) {
            // Flip the game background logic state
            isBackgroundChanged = !isBackgroundChanged;
            
            if (isBackgroundChanged) {
                LOGI("Screen Clicked! Changing background color to BLUE.");
                // Insert engine code here to render a blue background surface
            } else {
                LOGI("Screen Clicked! Resetting background color to BLACK.");
                // Insert engine code here to render a black background surface
            }
            return 1; // Input handled successfully
        }
    }
    return 0; // Ignore non-touch events
}

// Main execution loop for your app
void android_main(struct android_app* state) {
    LOGI("Pure C++ Game Engine Engine Booted.");
    
    // Tell Android to pass screen touch actions down to our input function above
    state->onInputEvent = handle_input;

    while (true) {
        int events;
        struct android_poll_source* source;

        // Process active operating system inputs
        if (ALooper_pollAll(-1, nullptr, &events, (void**)&source) >= 0) {
            if (source != nullptr) {
                source->process(state, source);
            }
        }

        // Keep updating the frame buffers or checking life-cycle destruction here
        if (state->destroyRequested != 0) {
            LOGI("Exiting engine system.");
            return;
        }
    }
}
