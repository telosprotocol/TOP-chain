#add_compile_options(-static -Wall -Werror)
add_compile_options(-static)

include_directories(.)
include_directories(./src)

aux_source_directory(./src src_dir)
add_library(xtxpoolsvr STATIC ${src_dir})

#add_dependencies(xtxpoolsvr xtxpool xxbase xstore xverifier)

target_link_libraries(xtxpoolsvr PRIVATE
    xtxpool
    xxbase
    xstore
    xverifier
)

if (BUILD_METRICS)
    #add_dependencies(xtxpoolsvr xmetrics)
    target_link_libraries(xtxpoolsvr PRIVATE xmetrics)
endif()

#if (XENABLE_TESTS)
#    enable_testing()
#    add_subdirectory(test/unittest)
#endif()
