cmake_minimum_required(VERSION 3.10)

project(ZnSerialize)

set(CMAKE_CXX_STANDARD 11)

add_library(ZnSerialize INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include/ZnSerialize/zn_serialize.hpp")
target_include_directories(ZnSerialize INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")
if (MSVC)
    target_compile_options(ZnSerialize INTERFACE /wd4819)
endif()

if(NOT ZnSerialize_DISABLE_TEST)
    add_executable(ZnSerializeTest test.cpp)
    target_link_libraries(ZnSerializeTest PRIVATE ZnSerialize)
endif()