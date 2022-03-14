cmake_minimum_required(VERSION 3.8)

aux_source_directory(./ xevm_runtime_src)

add_library(xevm_runtime STATIC ${xevm_runtime_src})

