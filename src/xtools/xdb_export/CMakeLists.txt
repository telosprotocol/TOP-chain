cmake_minimum_required(VERSION 3.8)

find_package(OpenMP)
add_compile_options(-fopenmp)

aux_source_directory(./src xdb_export_src)

add_executable(xdb_export ${xdb_export_src})

target_link_libraries(xdb_export PRIVATE xrpc xdbstore xstore xblockstore xtxstore xtxexecutor xmigrate xloader xdata xvledger xxbase OpenMP::OpenMP_CXX)

