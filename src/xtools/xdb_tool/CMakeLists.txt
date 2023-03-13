aux_source_directory(./src xdb_tool_src)

add_executable(xdb_tool ${xdb_tool_src})

target_link_libraries(xdb_tool xcertauth xblockstore xstore xdata xcodec xxbase)