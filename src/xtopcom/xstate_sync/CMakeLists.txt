aux_source_directory(./src src_dir)
add_library(xstate_sync STATIC ${src_dir})
add_dependencies(xstate_sync xdata xcommon xconfig xbasic xxbase)
target_link_libraries(xstate_sync PRIVATE xdata xxbase pthread)