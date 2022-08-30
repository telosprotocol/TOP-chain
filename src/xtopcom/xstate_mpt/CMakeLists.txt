aux_source_directory(./src src_dir)
add_library(xstate_mpt STATIC ${src_dir})
add_dependencies(xstate_mpt xdata xcommon xconfig xbasic xxbase)
target_link_libraries(xstate_mpt PRIVATE xdata xxbase pthread)