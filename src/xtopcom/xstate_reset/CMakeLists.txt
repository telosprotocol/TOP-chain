# add_compile_options(-Wpedantic)

aux_source_directory(./src xstate_reset_src)

add_library(xstate_reset STATIC ${xstate_reset_src})

target_link_libraries(xstate_reset PRIVATE xstatectx xdata xchain_fork xbasic)