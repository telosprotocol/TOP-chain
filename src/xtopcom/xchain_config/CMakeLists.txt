cmake_minimum_required(VERSION 3.8)

aux_source_directory(./src xchain_config_src)
add_library(xchain_config STATIC ${xchain_config_src})

#add_dependencies(xchain_config xvledger xutility xcodec xconfig xbasic)
target_link_libraries(xchain_config PRIVATE)
