#add_compile_options(-static -Wall -Werror)
add_compile_options(-static)

include_directories(.)
include_directories(./src)

aux_source_directory(./src src_dir)
add_library(xindexstore STATIC ${src_dir})

target_link_libraries(xindexstore PRIVATE
    xblockstore
    xstore
    xdata
    xxbase
)

if (BUILD_METRICS)
    target_link_libraries(xindexstore PRIVATE xmetrics)
endif()

if (XENABLE_TESTS)
    enable_testing()
endif()
