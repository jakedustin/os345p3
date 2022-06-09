cmake_minimum_required(VERSION 3.22)
project(os345_cmake C)

set(CMAKE_C_STANDARD 99)

add_executable(os345_cmake
        os345.c os345fat.c os345interrupts.c os345lc3.c os345mmu.c os345park.c os345semaphores.c os345signals.c os345tasks.c
        os345p1.c os345p2.c os345p3.c os345p4.c os345p5.c os345p6.c
        os345.h os345config.h os345fat.h os345lc3.h os345park.h os345signals.h)

add_executable(os345_scratch scratch.c)
