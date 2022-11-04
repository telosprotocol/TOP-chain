cmake_minimum_required(VERSION 3.8)

aux_source_directory(./src src_dir)

add_library(xca_auth ${src_dir})

target_link_libraries(xca_auth PRIVATE ssl crypto)

target_include_directories(xca_auth PRIVATE ${CMAKE_SOURCE_DIR}/src/xtopcom/xdepends/openssl_include/openssl/)
