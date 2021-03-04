#add_compile_options(-static -Wall -Werror)
add_compile_options(-static)

include_directories(.)
include_directories(./src)

aux_source_directory(./src src_dir)
add_library(xtxexecutor STATIC ${src_dir})

#add_dependencies(xtxexecutor xxbase xtxpool xblockstore xstore xverifier)

target_link_libraries(xtxexecutor PRIVATE
    xxbase
    xtxpool
    xblockstore
    xstore
    xvm
    xverifier
)

if (BUILD_METRICS)
    #add_dependencies(xtxexecutor xmetrics)
    target_link_libraries(xtxexecutor PRIVATE xmetrics)
endif()

if (XENABLE_TESTS)
    enable_testing()
    add_subdirectory(test/unittest)
endif()
