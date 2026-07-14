cmake_minimum_required(VERSION 3.10.2)
project("ArrowPuzzle")

set(CMAKE_CXX_STANDARD 17)

add_library(native-lib SHARED native-lib.cpp)

find_library(log-lib log)
find_library(jnigraphics-lib jnigraphics)

target_link_libraries(native-lib ${log-lib} ${jnigraphics-lib})
