#add_compile_options(-static -Wall -Werror)
add_compile_options(-static)

include_directories(.)
include_directories(./src)

aux_source_directory(./src src_dir)
add_library(xtxpool_v2 STATIC ${src_dir})

#add_dependencies(xtxpool_v2 xxbase xverifier xblockstore)

target_link_libraries(xtxpool_v2 PRIVATE
    xxbase
    xverifier
    xblockstore
)

if (BUILD_METRICS)
    #add_dependencies(xtxpool_v2 xmetrics)
    target_link_libraries(xtxpool_v2 PRIVATE xmetrics)
endif()


