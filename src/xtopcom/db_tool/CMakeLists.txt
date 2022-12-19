cmake_minimum_required(VERSION 3.8)

aux_source_directory(./ db_tool_src)
add_library(db_tool ${db_tool_src} )

find_library(RocksDB_LIBRARIES librocksdb.a REQUIRED)
find_library(zstd_LIBRARIES libzstd.a REQUIRED)
find_library(Snappy_LIBRARIES libsnappy.a REQUIRED)
find_library(minigzip_LIBRARIES libminizip.a REQUIRED)
find_library(lz4_LIBRARIES liblz4.a HINTS ${LIBRARY_OUTPUT_DIRECTORY})
find_package(BZip2 REQUIRED)
find_package(ZLIB REQUIRED)

add_dependencies(db_tool xpbase)
target_link_libraries(db_tool PRIVATE ${minigzip_LIBRARIES} ZLIB::ZLIB ${RocksDB_LIBRARIES} ${BZip2_LIBRARIES} ${zstd_LIBRARIES} ${Snappy_LIBRARIES} pthread)

