if (XBUILD_TEST)
    add_definitions(-DXENABLE_TESTS)
endif()

if (DISABLE_REAL_STAKE)
    add_definitions(-DXENABLE_MOCK_ZEC_STAKE)
endif()

if (MAINNET_ACTIVATED)
    add_definitions(-DMAINNET_ACTIVATED)
endif()

if (${XOS_ID} MATCHES "ubuntu")
    find_library(jsoncpp_LIBRARIES libjsoncpp.a HINTS ${LIBRARY_OUTPUT_DIRECTORY} REQUIRED)
endif()

aux_source_directory(./src src_dir)
add_library(xdata STATIC ${src_dir})

add_dependencies(xdata xconfig xutility xcommon xevm_common xcrypto xchain_fork xvledger xbasic xxbase)
if (${XOS_ID} MATCHES "centos")
    target_link_libraries(xdata PRIVATE xstatistic xconfig xutility xcommon xevm_common xcrypto xchain_fork xbasic xvledger jsoncpp xxbase pthread)
else()
    target_link_libraries(xdata PRIVATE xstatistic xconfig xutility xcommon xevm_common xcrypto xchain_fork xbasic xvledger xxbase ${jsoncpp_LIBRARIES} pthread)
endif()

if (BUILD_METRICS)
    add_dependencies(xdata xmetrics)
    target_link_libraries(xdata PRIVATE xmetrics)
endif()
