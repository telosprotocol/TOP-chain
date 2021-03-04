cmake_minimum_required(VERSION 3.8)

aux_source_directory(./ xthreadpool_test_dir)

add_executable(xthreadpool_test ${xthreadpool_test_dir})

#add_dependencies(xthreadpool_test xthreadpool)

target_link_libraries(xthreadpool_test
    uv
    -lgtest
    -lgmock
    -lpthread -ldl
)