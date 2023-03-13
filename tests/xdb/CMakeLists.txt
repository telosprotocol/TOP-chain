add_compile_options(-Wno-unused-parameter)

aux_source_directory(./ dir)
add_executable(xdb_test ${dir})

if (XDB_TYPE STREQUAL "ROCKSDB")

    if (${XSYS_VER} EQUAL 0)
        target_link_libraries(xdb_test
            xdb
            xxbase
            xvledger
            rocksdb zstd snappy bz2 z
            gtest
        )
    else()
        find_library(RocksDB_LIBRARIES rocksdb REQUIRED)
        find_library(lz4_LIBRARIES lz4 HINTS ${LIBRARY_OUTPUT_DIRECTORY} REQUIRED)
        find_library(zstd_LIBRARIES zstd REQUIRED)
        find_library(Snappy_LIBRARIES snappy REQUIRED)
        find_package(BZip2 REQUIRED)
        find_package(ZLIB REQUIRED)

        target_link_libraries(xdb_test
            xdb
            xxbase
            xvledger
            ${RocksDB_LIBRARIES}
            ${lz4_LIBRARIES}
            ${zstd_LIBRARIES}
            ${Snappy_LIBRARIES}
            ZLIB::ZLIB BZip2::BZip2
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
