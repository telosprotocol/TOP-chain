aux_source_directory(./ xloader_test_src)

add_executable(xloader_test ${xloader_test_src})

target_link_libraries(xloader_test PRIVATE xloader xstore xxbase gtest rocksdb z bz2 zstd snappy lz4 pthread)
