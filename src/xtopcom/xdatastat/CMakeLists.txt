#add_compile_options(-static -Wall -Werror)
add_compile_options(-static)

include_directories(.)
include_directories(./src)

aux_source_directory(./src src_dir)
add_library(xdatastat STATIC ${src_dir})

#add_dependencies(xdatastat xdata)

target_link_libraries(xdatastat PRIVATE xdata)

if (BUILD_METRICS)
    #add_dependencies(xdatastat xmetrics)
    target_link_libraries(xdatastat PRIVATE xmetrics)
endif()


