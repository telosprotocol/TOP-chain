if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CARGO_CMD cargo build)
    set(TARGET_DIR "debug")
else ()
    set(CARGO_CMD cargo build --release)
    set(TARGET_DIR "release")
endif ()

set(RUSTVM_A "${CMAKE_CURRENT_BINARY_DIR}/${TARGET_DIR}/librustvm.a")

add_custom_target(xrustvm ALL
    COMMENT "Compiling rustvm module"
    COMMAND ${CMAKE_COMMAND} -E echo "Force configure of rustvm"
    COMMAND CARGO_TARGET_DIR=${CMAKE_CURRENT_BINARY_DIR} ${CARGO_CMD} 
    # COMMAND echo ${RUSTVM_A} ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND cp ${RUSTVM_A} ${CMAKE_CURRENT_BINARY_DIR}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
set_target_properties(xrustvm PROPERTIES LOCATION ${CMAKE_CURRENT_BINARY_DIR})

add_test(NAME xrustvm_test 
    COMMAND cargo test
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_dependencies(xrustvm xcontract_api xcontract_api_SHARED)