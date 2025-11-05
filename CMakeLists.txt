cmake_minimum_required (VERSION 3.20)
project(webassembly-cpp-httplib-client-server LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_ALL "Cross-compile" ON)
option(BUILD_SERVER "Build server" OFF)
option(BUILD_CLIENT "Build client" OFF)

if(BUILD_ALL)
    set(EMSCRIPTEN_TOOLCHAIN_FILE "$ENV{EMSDK}/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake" CACHE FILEPATH "")
    cmake_path(SET EMSCRIPTEN_TOOLCHAIN_FILE NORMALIZE ${EMSCRIPTEN_TOOLCHAIN_FILE})

    set(PROJECT_TOP_BINARY_DIR ${CMAKE_BINARY_DIR} CACHE FILEPATH "")
    include(ExternalProject)

    ExternalProject_Add(server
        SOURCE_DIR ${CMAKE_SOURCE_DIR}
        CMAKE_CACHE_ARGS
            -DBUILD_ALL:BOOL=OFF
            -DBUILD_SERVER:BOOL=ON
            -DPROJECT_TOP_BINARY_DIR:FILEPATH=${PROJECT_TOP_BINARY_DIR}
        BUILD_ALWAYS ON
        INSTALL_COMMAND ""
    )

    ExternalProject_Add(client
        SOURCE_DIR ${CMAKE_SOURCE_DIR}
        CMAKE_CACHE_ARGS
            -DBUILD_ALL:BOOL=OFF
            -DBUILD_CLIENT:BOOL=ON
            -DPROJECT_TOP_BINARY_DIR:FILEPATH=${PROJECT_TOP_BINARY_DIR}
            -DCMAKE_TOOLCHAIN_FILE:FILEPATH=${EMSCRIPTEN_TOOLCHAIN_FILE}
        BUILD_ALWAYS ON
        INSTALL_COMMAND ""
    )

    return()
endif()

set(HTTPLIB_USE_OPENSSL_IF_AVAILABLE OFF)
set(HTTPLIB_USE_ZLIB_IF_AVAILABLE OFF)
set(HTTPLIB_USE_BROTLI_IF_AVAILABLE OFF)
set(HTTPLIB_USE_ZSTD_IF_AVAILABLE OFF)

include(FetchContent)
FetchContent_Declare(httplib
    GIT_REPOSITORY https://github.com/theo-dep/cpp-httplib
    GIT_TAG        49b526c1ec4a8c09bba0f5163c004024b5130a8c
)
FetchContent_MakeAvailable(httplib)

if(BUILD_SERVER)
    add_subdirectory(server)
endif()

if(BUILD_CLIENT)
    add_subdirectory(client)
endif()
