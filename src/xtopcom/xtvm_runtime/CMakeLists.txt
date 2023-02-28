add_compile_options(-Wpedantic)

aux_source_directory(./src src_dir)
add_library(xtvm_runtime STATIC ${src_dir})

add_dependencies(xtvm_runtime tvm_engine)
get_target_property(TVM_ENGINE_DIR tvm_engine LOCATION)
target_link_libraries(xtvm_runtime PRIVATE ${TVM_ENGINE_DIR}/libtvm_engine.a)
target_include_directories(xtvm_runtime PRIVATE ${CMAKE_SOURCE_DIR}/src/xtopcom/xtvm_engine_rs/tvm-c-api)

add_dependencies(xtvm_runtime xstate_accessor xevm_common xbasic xcommon)

target_link_libraries(xtvm_runtime PRIVATE tvm-c-api xstate_accessor xevm_common xbasic xcommon protobuf)
