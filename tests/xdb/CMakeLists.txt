add_compile_options(-Wno-unused-parameter)

aux_source_directory(./ dir)
add_executable(xdb_test ${dir})

if (XDB_TYPE STREQUAL "ROCKSDB")

    if (${XOS_ID} MATCHES "centos")
        target_link_libraries(xdb_test
            xdb
            xxbase
            xvledger
            rocksdb zstd snappy bz2 z
            gtest
        )
    else()
        target_link_libraries(xdb_test
            xdb
            xxbase
            xvledger
            ${rocksdb_LIBRARIES}
            gflags
            gtest
        )
    endif()

elseif (XDB_TYPE STREQUAL "LEVELDB")

    target_link_libraries(xdb_test PRIVATE
        xdb
        xxbase
        leveldb
    )

endif()
