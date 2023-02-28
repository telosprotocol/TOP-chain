aux_source_directory(./ tests_src)
aux_source_directory(./fixture fixture_src)

add_executable(xtvm_test ${tests_src} ${fixture_src})

add_dependencies(xtvm_test xtvm_runtime xdata xevm_common xbasic xcommon)

get_target_property(TVM_ENGINE_DIR tvm_engine LOCATION)
target_link_libraries(xtvm_test PRIVATE ${TVM_ENGINE_DIR}/libtvm_engine.a)
target_link_libraries(xtvm_test PRIVATE xtvm_runtime xdata xevm_common xbasic xcommon gtest pthread dl)