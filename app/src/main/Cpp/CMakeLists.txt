cmake_minimum_required(VERSION 3.10.2)
project("arrowpuzzle")

# Changed native-lib.cpp to main.cpp to match your file name
add_library(native-lib SHARED main.cpp)

find_library(log-lib log)

target_link_libraries(native-lib ${log-lib})
