cmake_minimum_required(VERSION 3.16)
project(gradium_s2s_realtime LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(gradium_s2s_realtime main.cpp)
target_link_libraries(gradium_s2s_realtime PRIVATE gradiumpp CLI11::CLI11 cppcodec)
