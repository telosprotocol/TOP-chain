if (DISABLE_REAL_STAKE)
    add_definitions(-DXENABLE_MOCK_ZEC_STAKE)
endif()

include_directories(.)

aux_source_directory(./src src_dir)
add_library(xgasfee STATIC ${src_dir})

add_dependencies(xgasfee xdata)
target_link_libraries(xgasfee PRIVATE xdata xxbase pthread)
