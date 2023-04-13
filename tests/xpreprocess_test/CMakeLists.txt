cmake_minimum_required(VERSION 3.8)

aux_source_directory(./ tests_xpreprocess)

add_executable(xpreprocess_test ${tests_xpreprocess})
target_link_libraries(xpreprocess_test PRIVATE xpreprocess xplugin xdata gtest)
if (BUILD_METRICS)
    target_link_libraries(xpreprocess_test PRIVATE xmetrics)
endif()