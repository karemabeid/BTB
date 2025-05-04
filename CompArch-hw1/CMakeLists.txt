cmake_minimum_required(VERSION 3.27)
project(CompArch_hw1)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)

add_executable(CompArch_hw1
        bp.c
        bp.cpp
        bp_api.h
        bp_main.c)
