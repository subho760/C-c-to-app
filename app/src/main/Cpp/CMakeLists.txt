cmake_minimum_required(VERSION 3.22.1)

project("native-lib")

add_library(
        native-lib
        SHARED
        main.cpp
)

find_library(
        log-lib
        log
)

# Add this inside your CMakeLists.txt to link the graphics libraries
target_link_libraries(
    # Your native library name (e.g., native-lib or backgroundchange)
    native-lib 

    # Add these three lines below to link Android graphics support:
    GLESv2
    EGL
    android
    log
)
