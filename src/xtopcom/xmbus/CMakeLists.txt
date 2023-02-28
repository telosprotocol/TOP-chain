aux_source_directory(./src src_dir)

add_library(xmbus STATIC ${src_dir})

add_dependencies(xmbus xevm_common xdata xcommon xvnetwork xbasic xvledger xxbase)
target_link_libraries(xmbus PRIVATE xevm_common xdata xvnetwork xbasic xvledger xxbase)

if (BUILD_METRICS)
    #add_dependencies(xmbus xmetrics)
    target_link_libraries(xmbus PRIVATE xmetrics)
endif()
