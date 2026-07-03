cmake_minimum_required(VERSION 3.22.1)
project("C-c-to-app")

add_library(
        C-c-to-app
        SHARED
        main.cpp
)

find_library(
        log-lib
        log
)

target_link_libraries(
        C-c-to-app
        ${log-lib}
)
