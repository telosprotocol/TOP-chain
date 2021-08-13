aux_source_directory(./ xsigner_src)
add_executable(xsigner_test ${xsigner_src})

#add_dependencies(xsigner_test xcommon)

target_link_libraries(xsigner_test PRIVATE trezor-crypto xcommon xpbase xxbase xcrypto -lgtest -lsecp256k1 -lpthread -ldl)
