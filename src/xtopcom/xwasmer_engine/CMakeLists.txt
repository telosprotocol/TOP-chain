cmake_minimum_required(VERSION 3.8)

add_compile_options(-Wpedantic)

message("========== wasm vm CMAKE ============")

# find cargo
execute_process (
    COMMAND bash -c "which cargo | grep 'cargo' | tr -d '\n'"
    OUTPUT_VARIABLE CARGO_DIR
)

message(STATUS "Cargo dir: " ${CARGO_DIR})
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
set(CARGO_CMD ${CARGO_DIR} build --features=build_as_xtop_lib)
set(TARGET_DIR "debug")
else ()
set(CARGO_CMD ${CARGO_DIR} build --features=build_as_xtop_lib --release)
set(TARGET_DIR "release")
endif ()

message(STATUS "Cargo cmd: " ${CARGO_CMD})
set(WASMER_VM_LIB "${CMAKE_CURRENT_BINARY_DIR}/${TARGET_DIR}/libxwasmer_engine.a")
message(STATUS "WASMER_VM_LIB: " ${WASMER_VM_LIB} )
message(STATUS "CMAKE_CURRENT_BINARY_DIR: " ${CMAKE_CURRENT_BINARY_DIR})
message(STATUS "CMAKE_CURRENT_SOURCE_DIR: " ${CMAKE_CURRENT_SOURCE_DIR})
message(STATUS "LIBRARY_OUTPUT_PATH: " ${LIBRARY_OUTPUT_PATH})

add_custom_target(xwasmer_engine ALL
    COMMAND CARGO_TARGET_DIR=${CMAKE_CURRENT_BINARY_DIR} ${CARGO_CMD} 
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND cp -f ${WASMER_VM_LIB} ${LIBRARY_OUTPUT_PATH})

set_target_properties(xwasmer_engine PROPERTIES LOCATION ${LIBRARY_OUTPUT_PATH})

add_dependencies(xwasmer_engine xwasmer_runner)