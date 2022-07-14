cmake_minimum_required(VERSION 3.8)

aux_source_directory(./ tests_src)

# compile definitions
add_definitions(
    -DUSE_STANDALONE_ASIO
    -DASIO_STANDALONE
)

add_executable(xtopcl_test ${tests_src})

add_dependencies(xtopcl_test xtopcl xcommon)

target_link_libraries(xtopcl_test PRIVATE xtopcl gtest pthread dl)