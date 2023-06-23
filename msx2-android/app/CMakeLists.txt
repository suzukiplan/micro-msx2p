cmake_minimum_required(VERSION 3.4.1)

add_library(
        native-lib
        SHARED
        src/main/cpp/jni.cpp
        src/main/cpp/core/emu2413.c
        src/main/cpp/lz4/lz4.c
)

include_directories(
        src/main/cpp
        src/main/cpp/core
        src/main/cpp/lz4
)

target_link_libraries(
        native-lib
        android
        jnigraphics
        OpenSLES
		log
)

