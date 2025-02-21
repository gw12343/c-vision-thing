cmake_minimum_required(VERSION 3.28)
project(c_vision_thing C)

set(CMAKE_C_STANDARD 11)

add_executable(c_vision_thing main.c
        vendor/stb_image.h
        vendor/stb_image_write.h
        util.h
        util.c)
