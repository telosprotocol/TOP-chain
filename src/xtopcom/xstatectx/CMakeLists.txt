#add_compile_options(-static -Wall -Werror)
add_compile_options(-static)

include_directories(.)
include_directories(./src)

aux_source_directory(./src src_dir)
add_library(xstatectx STATIC ${src_dir})

target_link_libraries(xstatectx PRIVATE
    xvledger
    xblockstore
    xdata
    xxbase
)

if (BUILD_METRICS)
    target_link_libraries(xstatectx PRIVATE xmetrics)
endif()
