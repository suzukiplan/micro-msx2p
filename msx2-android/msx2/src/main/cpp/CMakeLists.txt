cmake_minimum_required(VERSION 3.18.1)

project("msx2")

add_library(
    msx2
    SHARED
    msx2.cpp
    ../../../../../src/emu2413.c
    ../../../../../src/lz4.c
)

include_directories(
    ../../../../../src
)

target_link_libraries(
    msx2
    android
    jnigraphics
    OpenSLES
    log
)

