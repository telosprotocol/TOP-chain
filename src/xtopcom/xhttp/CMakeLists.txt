cmake_minimum_required(VERSION 3.8)

add_compile_options(-Wpedantic)

aux_source_directory(./client client_src)
aux_source_directory(./server server_src)

add_library(xhttp STATIC ${client_src} ${server_src})

target_include_directories(xhttp PRIVATE ${CMAKE_SOURCE_DIR}/src/xtopcom/xdepends/openssl_include/openssl/)

target_link_libraries(xhttp PRIVATE xbasic ssl crypto)