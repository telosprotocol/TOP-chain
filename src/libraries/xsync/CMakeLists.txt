if (XBUILD_TEST)
    add_definitions(-DSYNC_TEST)
endif()

aux_source_directory(./ src_dir)
aux_source_directory(./src src_dir)

add_library(xsync STATIC ${src_dir})

# add_dependencies(xsync xvnetwork xdata xbasic)

target_link_libraries(xsync PRIVATE 
    xvnetwork xdata xbasic 
)

IF (CMAKE_SYSTEM_NAME MATCHES "Darwin")
    target_link_libraries(xsync PRIVATE xvnetwork xdata xbasic)
ELSE()
    target_link_libraries(xsync PRIVATE xvnetwork xdata xbasic intel_aes64)
ENDIF()

if (BUILD_METRICS)
    # add_dependencies(xsync xmetrics)
    target_link_libraries(xsync PRIVATE xmetrics)
endif()

