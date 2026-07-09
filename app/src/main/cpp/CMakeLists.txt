cmake_minimum_required(VERSION 3.22.1)

project("backgroundchange")

add_library(
        backgroundchange
        SHARED
        game_logic.cpp
)

find_library(
        log-lib
        log
)

target_link_libraries(
        backgroundchange
        ${log-lib}
)
