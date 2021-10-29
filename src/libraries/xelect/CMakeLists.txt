cmake_minimum_required(VERSION 3.8)

add_compile_options(-static)

aux_source_directory(./src src_dir)

add_library(xelect STATIC ${src_dir})

add_dependencies(xelect xconfig xvnetwork xdata xvm xstore xrouter xxbase)
target_link_libraries(xelect PRIVATE xconfig xvnetwork xdata xvm protobuf xstore xrouter xxbase pthread)
