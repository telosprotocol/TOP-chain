cmake_minimum_required(VERSION 3.8)

aux_source_directory(./src xvnetwork_src)
add_library(xvnetwork STATIC ${xvnetwork_src})

#add_dependencies(xvnetwork xconfig xcodec xchain_timer xcommon xdata xelect_common xxbase xelection)
target_link_libraries(xvnetwork PRIVATE xconfig xcodec xchain_timer xelection xcommon xdata xvm xelect_common xxbase)

if (BUILD_METRICS)
    #add_dependencies(xvnetwork xmetrics)
    target_link_libraries(xvnetwork PRIVATE xmetrics)
endif()
