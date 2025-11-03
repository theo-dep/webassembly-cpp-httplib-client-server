cmake_minimum_required (VERSION 3.20)
project(webassembly-cpp-httplib-client-server LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(HTTPLIB_USE_OPENSSL_IF_AVAILABLE OFF)
set(HTTPLIB_USE_ZLIB_IF_AVAILABLE OFF)
set(HTTPLIB_USE_BROTLI_IF_AVAILABLE OFF)
set(HTTPLIB_USE_ZSTD_IF_AVAILABLE OFF)

include(FetchContent)
FetchContent_Declare(httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib
    GIT_TAG        v0.27.0
)
FetchContent_MakeAvailable(httplib)

add_subdirectory(server)
add_subdirectory(client)
