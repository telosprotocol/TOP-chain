#add_compile_options(-static -Wall -Werror)
add_compile_options(-static)

include_directories(.)
include_directories(./src)

aux_source_directory(./src src_dir)
add_library(xBFT STATIC ${src_dir})

#add_dependencies(xBFT xxbase)

target_link_libraries(xBFT PRIVATE
    xvledger
    xxbase
)

if (XENABLE_CODE_COVERAGE)
target_link_libraries(xBFT PRIVATE gcov)
endif()

if (BUILD_METRICS)
    #add_dependencies(xBFT xmetrics)
    target_link_libraries(xBFT PRIVATE xmetrics)
endif()

