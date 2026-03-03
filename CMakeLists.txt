cmake_minimum_required(VERSION 3.30)

project(ZnSerialize)

set(CMAKE_CXX_STANDARD 11)

add_executable(ZnSerializeTest zn_serialize.hpp test.cpp)
