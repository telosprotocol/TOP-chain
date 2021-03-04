#add_compile_options(-static -Wall -Werror)
add_compile_options(-static)

include_directories(.)
include_directories(./src)

aux_source_directory(./src src_dir)
add_library(xtxpool STATIC ${src_dir})

#add_dependencies(xtxpool xxbase xstore xverifier xblockstore)

target_link_libraries(xtxpool PRIVATE
    xxbase
    xstore
    xverifier
    xblockstore
)

if (BUILD_METRICS)
    #add_dependencies(xtxpool xmetrics)
    target_link_libraries(xtxpool PRIVATE xmetrics)
endif()


