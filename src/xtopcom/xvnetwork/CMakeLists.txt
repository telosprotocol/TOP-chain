aux_source_directory(./src xvnetwork_src)
add_library(xvnetwork STATIC ${xvnetwork_src})

add_dependencies(xvnetwork xconfig xcodec xchain_timer xelection xcommon xdata xxbase)
target_link_libraries(xvnetwork PRIVATE xconfig xcodec xchain_timer xelection xcommon xdata xxbase)

if (BUILD_METRICS)
    add_dependencies(xvnetwork xmetrics)
    target_link_libraries(xvnetwork PRIVATE xmetrics)
endif()
