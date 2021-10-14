add_compile_options(-Wno-unused-parameter)

aux_source_directory(./ xrocksdb_merge_demo_dir)
add_executable(xrocksdb_merge_demo ${xrocksdb_merge_demo_dir})

target_link_libraries(xrocksdb_merge_demo xxbase rocksdb zstd snappy bz2 z gtest gtest_main)
