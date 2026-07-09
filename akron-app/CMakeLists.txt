cmake_minimum_required(VERSION 3.20.0)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(BOARD esp32c3_devkitc) # default board
find_package(Zephyr)
project(arkon-app)

target_sources(app PRIVATE src/main.c)
