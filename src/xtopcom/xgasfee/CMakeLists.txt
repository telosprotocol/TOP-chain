add_compile_options(-static)

include_directories(.)

aux_source_directory(./src src_dir)
add_library(xgasfee STATIC ${src_dir})

# add_dependencies(xstate_accessor xdata xcommon xconfig xbasic xxbase)
target_link_libraries(xgasfee PRIVATE xdata xxbase pthread)
