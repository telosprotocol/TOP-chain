#add_compile_options(-static -Wall -Werror)
add_compile_options(-static)

include_directories(.)
include_directories(./src)

aux_source_directory(./src src_dir)
add_library(xtxpoolsvr_v2 STATIC ${src_dir})

add_dependencies(xtxpoolsvr_v2 xtxpool_v2 xxbase xstore xverifier)

target_link_libraries(xtxpoolsvr_v2 PRIVATE
    xtxpool_v2
    xxbase
    xstore
    xverifier
)

if (BUILD_METRICS)
    add_dependencies(xtxpoolsvr_v2 xmetrics)
    target_link_libraries(xtxpoolsvr_v2 PRIVATE xmetrics)
endif()

#if (XENABLE_TESTS)
#    enable_testing()
#    add_subdirectory(test/unittest)
#endif()
