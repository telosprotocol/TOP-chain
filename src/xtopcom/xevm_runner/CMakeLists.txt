cmake_minimum_required(VERSION 3.8)

aux_source_directory(./ xevm_runner_src)

add_library(xevm_runner STATIC ${xevm_runner_src})

