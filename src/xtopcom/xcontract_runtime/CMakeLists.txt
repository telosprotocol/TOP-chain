add_compile_options(-Wpedantic)

aux_source_directory(./src src_dir)
add_library(xcontract_runtime STATIC ${src_dir})

add_dependencies(xcontract_runtime xcontract_common xdata xcommon xbasic xxbase)

target_link_libraries(xcontract_runtime PRIVATE xcontract_common xdata xcommon xbasic xxbase pthread)

if (BUILD_RUSTVM)
    get_target_property(RUST_VM_DIR xrustvm LOCATION)
    target_link_libraries(xcontract_runtime PRIVATE ${RUST_VM_DIR}/librustvm.a)
    add_dependencies(xcontract_runtime xrustvm)
endif()
