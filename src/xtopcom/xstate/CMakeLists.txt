add_compile_options(-Wpedantic)

aux_source_directory(./src src_dir)
add_library(xstate STATIC ${src_dir})

add_dependencies(xstate xdata xcommon xconfig xbasic xxbase)

target_link_libraries(xstate PRIVATE xdata xcommon xconfig xbasic xxbase pthread)
