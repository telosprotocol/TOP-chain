aux_source_directory(./src src_dir)

add_library(xrouter STATIC ${src_dir})

add_dependencies(xrouter xcommon xdata xvledger xconfig xbasic)

target_link_libraries(xrouter PRIVATE xcommon xdata xvledger xconfig xbasic)

