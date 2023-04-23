if (${XOS_ID} MATCHES "ubuntu")
    find_library(secp256k1_LIBRARY libsecp256k1.a REQUIRED)
    find_library(cryptopp_LIBRARY libcryptopp.a REQUIRED)
endif()

if (DISABLE_REAL_STAKE)
    add_definitions(-DXENABLE_MOCK_ZEC_STAKE)
endif()

set(PROJ_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(SRC_DIR ${PROJ_DIR}/src)
aux_source_directory(${SRC_DIR} cppsdk_src)
aux_source_directory(${SRC_DIR}/base cppsdk_src)
aux_source_directory(${SRC_DIR}/network cppsdk_src)
aux_source_directory(${SRC_DIR}/task cppsdk_src)

if (${XOS_ID} MATCHES "centos")
    set(LINK_ARGS secp256k1 trezor-crypto cryptopp)
else()
    set(LINK_ARGS ${secp256k1_LIBRARY} trezor-crypto ${cryptopp_LIBRARY})
endif()

if(NOT MSVC)
    set(LINK_ARGS ${LINK_ARGS} pthread)
endif()

# SET TARGET
add_library(xtopcl STATIC ${cppsdk_src})
add_dependencies(xtopcl xconfig xdata xxbase xelect_net)

target_link_libraries(xtopcl PRIVATE ${LINK_ARGS} xsafebox xconfig xdata xelect_net xxbase jsoncpp)

