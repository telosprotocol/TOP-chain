aux_source_directory(./ src_dir)
add_executable(xmake_keys ${src_dir})

target_link_libraries(xmake_keys
    xxbase
    xcrypto
    xutility
    -pthread -lz -ldl
)
