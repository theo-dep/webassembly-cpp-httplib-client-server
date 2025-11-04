add_executable(server main.cpp)
target_link_libraries(server PRIVATE httplib::httplib)
set_target_properties(server PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_TOP_BINARY_DIR})
