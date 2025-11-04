add_executable(client main.cpp)
target_compile_options(client PRIVATE -sMEMORY64 -pthread)
target_link_options(client PRIVATE -sMEMORY64 -pthread -lwebsocket.js -sPROXY_POSIX_SOCKETS -sPROXY_TO_PTHREAD -sEXIT_RUNTIME)
target_include_directories(client PRIVATE ${httplib_SOURCE_DIR})
set_target_properties(client
    PROPERTIES
        SUFFIX .html
        RUNTIME_OUTPUT_DIRECTORY ${PROJECT_TOP_BINARY_DIR}
)
