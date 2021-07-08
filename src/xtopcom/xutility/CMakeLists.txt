aux_source_directory(./src xutility_src)
add_library(xutility STATIC ${xutility_src})

# #add_dependencies(xutility )

target_link_libraries(xutility PRIVATE trezor-crypto)
