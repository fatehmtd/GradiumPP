cmake_minimum_required(VERSION 3.16)
project(gradium_tts_rest LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(gradium_tts_rest main.cpp)
target_link_libraries(gradium_tts_rest PRIVATE gradiumpp CLI11::CLI11)
