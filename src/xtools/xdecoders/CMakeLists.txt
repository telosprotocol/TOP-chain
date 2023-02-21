cmake_minimum_required(VERSION 3.8)

aux_source_directory(./src tool_src)

add_executable(top_binary_data_decoder ${tool_src})

add_dependencies(top_binary_data_decoder xbasic)
target_link_libraries(top_binary_data_decoder xbasic xbase)
