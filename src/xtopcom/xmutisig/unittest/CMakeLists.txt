cmake_minimum_required(VERSION 3.8)

aux_source_directory(./test_mutisig unit_src)
aux_source_directory(./test_serialize unit_src)
aux_source_directory(./test_performance unit_src)
aux_source_directory(./ unit_src)
add_executable(mutisig_test ${unit_src})

#add_dependencies(mutisig_test xmutisig)

target_link_libraries(mutisig_test PRIVATE xmutisig gtest xcrypto xutility)