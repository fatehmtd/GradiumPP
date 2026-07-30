cmake_minimum_required(VERSION 3.16)
project(gradiumpp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# Library target
# ---------------------------------------------------------------------------

if(WIN32)
    set(GRADIUM_WS_TRANSPORT_SRC src/transport/winhttp_websocket_transport.cpp)
else()
    set(GRADIUM_WS_TRANSPORT_SRC src/transport/lws_websocket_transport.cpp)
endif()

add_library(gradiumpp STATIC
    src/tts_client.cpp
    src/asr_client.cpp
    src/s2s_client.cpp
    src/voice_client.cpp
    src/pronunciation_client.cpp
    src/credits_client.cpp
    src/transport/curl_http_transport.cpp
    ${GRADIUM_WS_TRANSPORT_SRC}
)

target_include_directories(gradiumpp
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(gradiumpp
    PRIVATE
        libcurl
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        cppcodec
)

if(WIN32)
    target_compile_definitions(gradiumpp PRIVATE _WIN32_WINNT=0x0602 WIN32_LEAN_AND_MEAN)
    target_link_libraries(gradiumpp PRIVATE winhttp ws2_32 crypt32)
else()
    target_link_libraries(gradiumpp PRIVATE websockets)
endif()

# ---------------------------------------------------------------------------
# Install rules
# ---------------------------------------------------------------------------

include(GNUInstallDirs)

install(
    TARGETS gradiumpp
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(
    DIRECTORY include/gradiumpp
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
