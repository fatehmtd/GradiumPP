cmake_minimum_required(VERSION 3.16)
project(gradium_asr_realtime LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(gradium_asr_realtime main.cpp)
target_link_libraries(gradium_asr_realtime PRIVATE gradiumpp CLI11::CLI11 cppcodec)
