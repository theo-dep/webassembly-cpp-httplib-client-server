add_subdirectory(websocket_to_posix_proxy)

add_executable(server main.cpp)
target_link_libraries(server PRIVATE httplib::httplib websocket_to_posix_proxy)
set_target_properties(server PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_TOP_BINARY_DIR})
