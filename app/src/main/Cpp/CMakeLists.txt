cmake_minimum_required(VERSION 3.10.2)
project("arrowpuzzle")

# Force the compiler environment to strictly use C++17 rules
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Link your main code file (matching your Cpp directory and file name)
add_library(native-lib SHARED main.cpp)

find_library(log-lib log)

target_link_libraries(native-lib ${log-lib})
