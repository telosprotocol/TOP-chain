cmake_minimum_required(VERSION 3.8)

# aux_source_directory(./src metrics_src)

add_executable(metrics_test ./test_metrics.cpp)

#add_dependencies(metrics_test xmetrics)
target_link_libraries(metrics_test PRIVATE xmetrics gtest gtest_main pthread)