aux_source_directory(./test_mutisig unit_src)
aux_source_directory(./test_serialize unit_src)
aux_source_directory(./test_performance unit_src)
aux_source_directory(./ unit_src)
add_executable(xmutisig_test ${unit_src})

#add_dependencies(xmutisig_test xmutisig)

if (${XOS_ID} MATCHES "centos")
target_include_directories(xmutisig_test PRIVATE ${CMAKE_SOURCE_DIR}/src/xtopcom/xdepends/openssl_include/openssl/)
endif()
target_link_libraries(xmutisig_test PRIVATE xmutisig gtest xcrypto xutility)