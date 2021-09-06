cmake_minimum_required(VERSION 3.8)

aux_source_directory(./src xpara_proxy_src)
add_library(xpara_proxy STATIC ${xpara_proxy_src})


add_definitions(
    -DUSE_STANDALONE_ASIO
    -DASIO_STANDALONE
    # -D__STRICT_ANSI__
)

# add_dependencies(xpara_proxy xvnode xloader xconfig xelect xelect_net xchain_timer xvnetwork xelection xblockmaker xtxexecutor xunit_service xtxpoolsvr_v2 xBFT xsync xtopcl xmbus xelect_common xbasic xcertauth)
IF (CMAKE_SYSTEM_NAME MATCHES "Darwin")
    target_link_libraries(xpara_proxy PRIVATE xvnode xloader xconfig xelect xelect_net xchain_timer xvnetwork xblockmaker xtxexecutor xunit_service xtxpoolsvr_v2 xBFT xelection xrpc xsync xtopcl xmbus xelect_common xbasic xcertauth)
ELSE()
    target_link_libraries(xpara_proxy PRIVATE xvnode xloader xconfig xelect xelect_net xchain_timer xvnetwork xblockmaker xtxexecutor xunit_service xtxpoolsvr_v2 xBFT xelection xrpc xsync xtopcl xmbus xelect_common xbasic xcertauth intel_aes64)
ENDIF()
