cmake_minimum_required(VERSION 3.8)

aux_source_directory(./ db_tool_src)
add_library(db_tool ${db_tool_src} )

#add_dependencies(db_tool xpbase)
target_link_libraries(db_tool PRIVATE minizip z rocksdb pthread lz4 bz2 zstd snappy)

