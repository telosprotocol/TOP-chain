cmake_minimum_required(VERSION 3.8)

aux_source_directory(./src src_dir)

add_library(xchain_upgrade STATIC ${src_dir})

#add_dependencies(xchain_upgrade xbasic xdata xconfig)

target_link_libraries(xchain_upgrade PRIVATE xbase xbasic xdata pthread)
