cmake_minimum_required(VERSION 3.10.2)
project("arrowpuzzle")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# This tells CMake to look for the file directly in its own directory
add_library(native-lib SHARED ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)

find_library(log-lib log)
target_link_libraries(native-lib ${log-lib})
