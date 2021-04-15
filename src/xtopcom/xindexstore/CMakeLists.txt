#add_compile_options(-static -Wall -Werror)
add_compile_options(-static)

include_directories(.)
include_directories(./src)

aux_source_directory(./src src_dir)
add_library(xindexstore STATIC ${src_dir})

IF (CMAKE_SYSTEM_NAME MATCHES "Darwin")
    target_link_libraries(xindexstore PRIVATE
        xblockstore
        xstore
        xdata
        xxbase
    )
ELSE()
    target_link_libraries(xindexstore PRIVATE
        xblockstore
        xstore
        xdata
        xxbase
        intel_aes64
    )
ENDIF()

if (BUILD_METRICS)
    target_link_libraries(xindexstore PRIVATE xmetrics)
endif()

if (XENABLE_TESTS)
    enable_testing()
endif()
