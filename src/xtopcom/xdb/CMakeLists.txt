#cmake_minimum_required(VERSION 2.8)

# add_compile_options(-static)

# aux_source_directory(./src src_dir)
# add_library(xdb STATIC ${src_dir})

message(STATUS "XDB_TYPE:" ${XDB_TYPE})
find_library(RocksDB_LIBRARIES librocksdb.a REQUIRED)
find_library(zstd_LIBRARIES libzstd.a REQUIRED)
find_library(Snappy_LIBRARIES libsnappy.a REQUIRED)
find_package(BZip2 REQUIRED)
find_package(ZLIB REQUIRED)

if (XDB_TYPE STREQUAL "ROCKSDB")
    add_library(xdb STATIC
        ./src/xdb_factory.cpp
        ./src/xdb_rocksdb.cpp
        ./src/xdb_memdb.cpp
    )
    add_dependencies(xdb xxbase)

    target_link_libraries(xdb PRIVATE
        xxbase
        ${RocksDB_LIBRARIES} ${zstd_LIBRARIES} ${Snappy_LIBRARIES} ${BZIP2_LIBRARIES} ZLIB::ZLIB
    )

elseif (XDB_TYPE STREQUAL "LEVELDB")
    add_library(xdb STATIC
        ./src/xdb_factory.cpp
        ./src/xdb_leveldb.cpp
        ./src/xdb_memdb.cpp
    )
    #add_dependencies(xdb xxbase)

    target_link_libraries(xdb PRIVATE xxbase leveldb)

endif()

if (BUILD_METRICS)
    target_link_libraries(xdb PRIVATE xmetrics)
endif()