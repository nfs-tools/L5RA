cmake_minimum_required(VERSION 3.8)
project(L5RA)

set(CMAKE_CXX_STANDARD 17)

set(SOURCE_FILES main.cpp MooParser.cc MooParser.h MooFile.cc MooFile.h FileStream.cc FileStream.h)
add_executable(L5RA ${SOURCE_FILES})