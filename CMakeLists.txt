cmake_minimum_required(VERSION 3.16)
project(soundTest C)

set(CMAKE_C_STANDARD 99)

find_path(PULSEAUDIO_INCLUDE_DIR
        NAMES pulse/pulseaudio.h
        DOC "The PulseAudio include directory"
        )
find_library(PULSEAUDIO_LIBRARY
        NAMES pulse
        DOC "The PulseAudio library"
        )
include_directories(${PULSEAUDIO_INCLUDE_DIRS})

add_executable(soundTest main.c)

target_link_libraries(soundTest pulse-simple pulse)
target_link_libraries(soundTest m)
target_link_libraries(soundTest fftw3)