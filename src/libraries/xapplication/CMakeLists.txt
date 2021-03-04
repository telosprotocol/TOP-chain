cmake_minimum_required(VERSION 3.8)

aux_source_directory(./src xapplication_src)
add_library(xapplication STATIC ${xapplication_src})


add_definitions(
    -DUSE_STANDALONE_ASIO
    -DASIO_STANDALONE
    # -D__STRICT_ANSI__
)

#add_dependencies(xapplication xvnode xloader xconfig xelect xchain_timer xvnetwork xelection xtxexecutor xunit_service xtxpoolsvr xBFT xsync xtopcl xmbus xelect_common xbasic xcertauth xdatastat)
target_link_libraries(xapplication PRIVATE xvnode  xvm xloader xconfig xelect xelect_net xstake xchain_timer xvnetwork xtxexecutor xunit_service xtxpoolsvr xBFT xelection xrpc xsync xtopcl xmbus xelect_common xbasic xcertauth xdatastat)
