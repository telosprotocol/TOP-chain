cmake_minimum_required(VERSION 3.8)

aux_source_directory(./ xca_test_src)
add_executable(xca_test ${xca_test_src})

#add_dependencies(xca_test xBFT xxbase xcertauth xutility xcrypto)

target_link_libraries(xca_test PRIVATE xxbase xcertauth xutility xcrypto gtest gmock)
