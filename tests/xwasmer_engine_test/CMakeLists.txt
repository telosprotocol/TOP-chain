cmake_minimum_required(VERSION 3.8)

aux_source_directory(./ tests_src)

add_executable(xwasmer_engine_test ${tests_src})

add_dependencies(xwasmer_engine_test xwasmer_engine xcommon)
get_target_property(WASMER_ENGINE_DIR xwasmer_engine LOCATION)
message("WASMER_ENGINE_DIR: " ${WASMER_ENGINE_DIR})
target_link_libraries(xwasmer_engine_test PRIVATE xcommon ${WASMER_ENGINE_DIR}/libxwasmer_engine.a xwasmer_runner gtest pthread dl)