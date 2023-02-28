aux_source_directory(./src src_dir)

add_library(xchain_timer STATIC ${src_dir})

add_dependencies(xchain_timer xconfig xdata xbasic xxbase)

target_link_libraries(xchain_timer PRIVATE xconfig xdata xbasic xxbase pthread)

