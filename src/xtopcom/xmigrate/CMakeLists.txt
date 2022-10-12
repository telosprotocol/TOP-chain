include_directories(.)

aux_source_directory(./src src_dir)
add_library(xmigrate STATIC ${src_dir})

if (BUILD_METRICS)
    target_link_libraries(xmigrate PRIVATE xmetrics)
endif()

target_link_libraries(xmigrate PRIVATE
    xdata
    xvledger
    xdb
    xxbase
)
