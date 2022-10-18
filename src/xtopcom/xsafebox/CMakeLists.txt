cmake_minimum_required(VERSION 3.8)

add_compile_options(-Wpedantic)

aux_source_directory(./src safebox_src)

add_library(xsafebox STATIC ${safebox_src})

target_include_directories(xsafebox PRIVATE ${CMAKE_SOURCE_DIR}/src/xtopcom/xdepends/openssl_include/openssl/)

target_link_libraries(xsafebox PRIVATE xhttp xcrypto xbasic)