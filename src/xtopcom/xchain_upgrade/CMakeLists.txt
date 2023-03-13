aux_source_directory(./src src_dir)

add_library(xchain_upgrade STATIC ${src_dir})

add_dependencies(xchain_upgrade xdata xbasic)

target_link_libraries(xchain_upgrade PRIVATE xdata xbasic xbase uv dl pthread)
