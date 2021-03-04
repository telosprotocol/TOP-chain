#cmake_minimum_required(VERSION 2.8)

# add_compile_options(-static)

# aux_source_directory(./src src_dir)
# add_library(xdb STATIC ${src_dir})

message(STATUS "XDB_TYPE:" ${XDB_TYPE})

if (XDB_TYPE STREQUAL "ROCKSDB")
    add_library(xdb STATIC
        ./src/xdb_factory.cpp
        ./src/xdb_rocksdb.cpp
        ./src/xdb_memdb.cpp
    )
    #add_dependencies(xdb xxbase)

    target_link_libraries(xdb PRIVATE
        xxbase
        rocksdb zstd snappy bz2 z lz4
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

