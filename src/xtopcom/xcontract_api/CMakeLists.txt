# add_compile_options(-Wall -Werror -Wpedantic)

add_compile_options(-fprofile-arcs)
add_compile_options(-fPIC)
add_compile_options(-O3)
add_compile_options(-c)

aux_source_directory(./src src_dir)
# add_library(xcontract_api STATIC ${src_dir})
add_library(xcontract_api SHARED ${src_dir})

add_dependencies(xcontract_api xcontract_common xdata xcommon xbasic xxbase)

target_link_libraries(xcontract_api PRIVATE xcontract_common xdata xcommon xbasic xxbase pthread)
