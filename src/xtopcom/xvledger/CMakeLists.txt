include_directories(.)

aux_source_directory(./src src_dir)
add_library(xvledger STATIC ${src_dir})

if (BUILD_METRICS)
    # add_dependencies(xvledger xmetrics)
    target_link_libraries(xvledger PRIVATE xmetrics)
endif()

add_dependencies(xvledger xconfig xxbase)
target_link_libraries(xvledger PRIVATE
    xstatistic
    xconfig
    xxbase
    jsoncpp
)
