cmake_minimum_required(VERSION 3.8)

add_compile_options(-static)

aux_source_directory(./src src_dir)

add_library(xloader STATIC ${src_dir})

#add_dependencies(xloader xconfig xdata xbasic xpbase)

target_link_libraries(xloader PRIVATE jsoncpp xconfig xmbus xdata xbasic xpbase xblockstore)

