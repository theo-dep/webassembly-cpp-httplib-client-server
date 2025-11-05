add_library(websocket_to_posix_proxy STATIC
    src/connect_websocket.cpp
    src/sha1.cpp
    src/socket_registry.cpp
    src/websocket_to_posix_proxy.c
)

target_include_directories(websocket_to_posix_proxy PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if (WIN32)
    target_compile_definitions(websocket_to_posix_proxy PUBLIC _CRT_SECURE_NO_WARNINGS)
    target_compile_options(websocket_to_posix_proxy PRIVATE /wd4200) # "nonstandard extension used: zero-sized array in struct/union"
    target_link_libraries(websocket_to_posix_proxy PUBLIC Ws2_32)
else()
    target_compile_options(websocket_to_posix_proxy PRIVATE -Werror -Wall)
endif()

find_package(Threads)
target_link_libraries(websocket_to_posix_proxy PUBLIC ${CMAKE_THREAD_LIBS_INIT})
