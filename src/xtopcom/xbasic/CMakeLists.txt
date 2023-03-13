if (XBUILD_TEST)
    add_definitions(-DXENABLE_TESTS)
endif()

aux_source_directory(./src xbasic_src)
add_library(xbasic STATIC ${xbasic_src})
# IF (CMAKE_SYSTEM_NAME MATCHES "Darwin")
#     target_link_libraries(xbasic PRIVATE xbase uv dl pthread mbedtls mbedcrypto mbedx509)
# ELSE()
#     target_link_libraries(xbasic PRIVATE xbase uv dl pthread mbedtls mbedcrypto mbedx509 intel_aes64 rt atomic pthread)
# ENDIF()
target_link_libraries(xbasic PRIVATE xxbase)

