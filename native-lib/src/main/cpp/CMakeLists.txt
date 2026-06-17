
cmake_minimum_required(VERSION 3.22.1)
project("native-lib")

find_library(log-lib log)
find_library(android-lib android)

add_library(native-lib SHARED main.cpp)

# Links the essential native app glue library
target_link_libraries(native-lib
    ${log-lib}
    ${android-lib}
    android_native_app_glue
)
