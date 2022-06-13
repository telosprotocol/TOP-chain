cmake_minimum_required(VERSION 3.8)

add_compile_options(-Wpedantic)

aux_source_directory(./ xwasmer_runner_src)

add_library(xwasmer_runner STATIC ${xwasmer_runner_src})

add_dependencies(xwasmer_runner xdata xcontract_common)

target_link_libraries(xwasmer_runner PRIVATE xdata xcontract_common protobuf)
