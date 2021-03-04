#add_compile_options(-static -Wall -Werror)
add_compile_options(-static)

include_directories(.)
include_directories(./src)

aux_source_directory(./src src_dir)
add_library(xblockstore STATIC ${src_dir})

#add_dependencies(xblockstore xxbase xstore)

target_link_libraries(xblockstore PRIVATE
    xxbase
    xstore
)

# if (BUILD_METRICS)
#     #add_dependencies(xblockstore xmetrics)
#     target_link_libraries(xblockstore PRIVATE xmetrics)
# endif()

if (XENABLE_TESTS)
    enable_testing()
    #add_subdirectory(test/basic)
endif()
