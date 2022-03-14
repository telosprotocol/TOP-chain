if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CARGO_CMD cargo build --features=build_as_xtop_lib)
    set(TARGET_DIR "debug")
else ()
    set(CARGO_CMD cargo build --features=build_as_xtop_lib --release)
    set(TARGET_DIR "release")
endif ()
message("========== evm_engine CMAKE ============")
message(STATUS "Cargo cmd: " ${CARGO_CMD})
set(EVM_A "${CMAKE_CURRENT_BINARY_DIR}/${TARGET_DIR}/libxevm_engine.a")
message(STATUS "EVM_A: " ${EVM_A} )
message(STATUS "CMAKE_CURRENT_BINARY_DIR: " ${CMAKE_CURRENT_BINARY_DIR})
message(STATUS "CMAKE_CURRENT_SOURCE_DIR: " ${CMAKE_CURRENT_SOURCE_DIR})
message(STATUS "LIBRARY_OUTPUT_PATH: " ${LIBRARY_OUTPUT_PATH})

add_custom_target(xevm_engine ALL
    COMMAND CARGO_TARGET_DIR=${CMAKE_CURRENT_BINARY_DIR} ${CARGO_CMD} 
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND cp -f ${EVM_A} ${LIBRARY_OUTPUT_PATH})

set_target_properties(xevm_engine PROPERTIES LOCATION ${LIBRARY_OUTPUT_PATH})
# mock:
add_dependencies(xevm_engine xevm_runtime)