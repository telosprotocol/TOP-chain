aux_source_directory(./src xgossip_src)
add_library(xgossip STATIC ${xgossip_src})
add_dependencies(xgossip xkad)

target_link_libraries(xgossip PRIVATE xkad xpbase xtransport xwrouter)

if (XENABLE_P2P_TEST)
    target_compile_definitions(xgossip PRIVATE XENABLE_P2P_TEST)
endif()