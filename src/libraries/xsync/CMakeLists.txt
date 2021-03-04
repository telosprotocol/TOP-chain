cmake_minimum_required(VERSION 3.8)

add_compile_options(-static)

aux_source_directory(./ src_dir)
aux_source_directory(./src src_dir)

add_library(xsync STATIC ${src_dir})

#add_dependencies(xsync xvnetwork xdata xbasic)

target_link_libraries(xsync PRIVATE 
    xvnetwork xdata xbasic
)

if (BUILD_METRICS)
    #add_dependencies(xsync xmetrics)
    target_link_libraries(xsync PRIVATE xmetrics)
endif()

