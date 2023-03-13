if (XBUILD_TEST)
    add_definitions(-DXENABLE_TESTS)
endif()
if (DISABLE_REAL_STAKE)
    add_definitions(-DXENABLE_MOCK_ZEC_STAKE)
endif()

aux_source_directory(./src xcommon_src)
add_library(xcommon STATIC ${xcommon_src})

add_dependencies(xcommon xconfig xutility xcodec xbasic xvledger)
target_link_libraries(xcommon PRIVATE xconfig xutility xcodec xbasic xvledger ethash keccak)
