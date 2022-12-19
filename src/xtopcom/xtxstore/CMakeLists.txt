cmake_minimum_required(VERSION 3.8)
#add_compile_options(-static -Wall -Werror)
# add_compile_options(-Wpedantic)
add_compile_options(-static)

aux_source_directory(./src src_dir)

add_library(xtxstore STATIC ${src_dir})

add_dependencies(xtxstore xstore xcommon xxbase)

target_link_libraries(xtxstore PRIVATE xstore xtxexecutor xcommon xxbase)

if (BUILD_METRICS)
    target_link_libraries(xtxstore PRIVATE xmetrics)
endif()
