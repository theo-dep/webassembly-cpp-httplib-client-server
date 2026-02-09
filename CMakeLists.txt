cmake_minimum_required (VERSION 3.20)
project(webassembly-cpp-httplib-client-server LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_ALL "Cross-compile" ON)
option(BUILD_SERVER "Build server" OFF)
option(BUILD_CLIENT "Build client" OFF)

if(BUILD_ALL)
    execute_process(COMMAND ${CMAKE_COMMAND} --preset server WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
    execute_process(COMMAND ${CMAKE_COMMAND} --preset client WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

    add_custom_target(server ALL
        COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR}/server
        COMMENT "Building server"
    )

    add_custom_target(client ALL
        COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR}/client
        COMMENT "Building client"
    )

    return()
endif()

set(HTTPLIB_USE_OPENSSL_IF_AVAILABLE OFF)
set(HTTPLIB_USE_ZLIB_IF_AVAILABLE OFF)
set(HTTPLIB_USE_BROTLI_IF_AVAILABLE OFF)
set(HTTPLIB_USE_ZSTD_IF_AVAILABLE OFF)
add_subdirectory(httplib)

if(BUILD_SERVER)
    add_subdirectory(server)
endif()

if(BUILD_CLIENT)
    add_subdirectory(client)
endif()
