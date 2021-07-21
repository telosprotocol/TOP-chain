cmake_minimum_required(VERSION 3.8)

aux_source_directory(./src xaction_param_parse_src)

add_executable(xparse_tool ${xaction_param_parse_src})

target_link_libraries(xparse_tool xdata xxbase)