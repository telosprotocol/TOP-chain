add_compile_options(-static -Wall -Werror)

aux_source_directory(./src src_dir)
add_library(xblock STATIC ${src_dir})

add_dependencies(xblock xstatectx xdata xvledger)

target_link_libraries(xblock PRIVATE xstatectx xdata xvledger)

if (BUILD_METRICS)
    add_dependencies(xblock xmetrics)
    target_link_libraries(xblock PRIVATE xmetrics)
endif()
