add_compile_options(-Wno-unused-parameter)

aux_source_directory(./ dir)
add_executable(xdb_test ${dir})

if (XDB_TYPE STREQUAL "ROCKSDB")

    target_link_libraries(xdb_test
        xdb
        xxbase
        rocksdb zstd snappy bz2 z
        -lgtest
    )

elseif (XDB_TYPE STREQUAL "LEVELDB")

    target_link_libraries(xdb_test PRIVATE 
        xdb
        xxbase 
        leveldb
    )

endif()
