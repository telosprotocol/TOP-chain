cmake_minimum_required(VERSION 3.8)

aux_source_directory(./src xbasic_src)
add_library(xbasic STATIC ${xbasic_src})
# IF (CMAKE_SYSTEM_NAME MATCHES "Darwin")
#     target_link_libraries(xbasic PRIVATE xbase uv dl pthread mbedtls mbedcrypto mbedx509 msgpackc)
# ELSE()
#     target_link_libraries(xbasic PRIVATE xbase uv dl pthread mbedtls mbedcrypto mbedx509 intel_aes64 rt atomic msgpackc pthread)
# ENDIF()
target_link_libraries(xbasic PRIVATE xxbase msgpackc)

