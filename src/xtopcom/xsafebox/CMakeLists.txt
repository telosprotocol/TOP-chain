add_compile_options(-Wpedantic)

aux_source_directory(./src safebox_src)

add_library(xsafebox STATIC ${safebox_src})

if (${XSYS_VER} EQUAL 0)
    target_include_directories(xsafebox PRIVATE ${CMAKE_SOURCE_DIR}/src/xtopcom/xdepends/openssl_include/openssl/)
endif()

target_link_libraries(xsafebox PRIVATE xhttp xcrypto xbasic)
