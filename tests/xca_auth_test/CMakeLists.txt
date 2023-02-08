cmake_minimum_required(VERSION 3.8)

aux_source_directory(./ xca_auth_test_src)
add_executable(xca_auth_test ${xca_auth_test_src})


target_link_libraries(xca_auth_test PRIVATE xca_auth jsoncpp  xgenesis xxbase gtest gmock)
