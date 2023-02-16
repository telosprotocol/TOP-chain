add_compile_options(-static -Wall -Werror)

if (XBUILD_TEST)
    add_definitions(-DXENABLE_TESTS)
endif()

if (DISABLE_REAL_STAKE)
    add_definitions(-DXENABLE_MOCK_ZEC_STAKE)
endif()

if (MAINNET_ACTIVATED)
    add_definitions(-DMAINNET_ACTIVATED)
endif()

aux_source_directory(./src src_dir)
add_library(xdata STATIC ${src_dir})

add_dependencies(xdata xstatistic xconfig xutility xcommon xcrypto xbasic xxbase)

target_link_libraries(xdata PRIVATE xstatistic xconfig xutility xcommon xevm_common xverifier xcrypto xchain_fork xbasic xvledger jsoncpp xxbase pthread)

if (BUILD_METRICS)
    add_dependencies(xdata xmetrics)
    target_link_libraries(xdata PRIVATE xmetrics)
endif()
