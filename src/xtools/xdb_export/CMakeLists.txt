cmake_minimum_required(VERSION 3.8)

aux_source_directory(./src xdb_export_src)

add_executable(xdb_export ${xdb_export_src})

target_link_libraries(xdb_export xrpc xstore xblockstore xtxexecutor xmigrate xloader xdata xxbase)
