cmake_minimum_required(VERSION 3.8)

aux_source_directory(./ tests_plugins)

add_executable(xplugins_test ${tests_plugins})

target_link_libraries(xplugins_test PRIVATE xdata xutility xcrypto gtest pthread dl xplugin xplugin_audit)
if (BUILD_METRICS)
    target_link_libraries(xplugins_test PRIVATE xmetrics)
endif()