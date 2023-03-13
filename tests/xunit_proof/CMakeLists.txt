cmake_minimum_required(VERSION 3.8)

aux_source_directory(./ xunit_proof_test_src)

add_executable(xunit_proof_test ${xunit_proof_test_src})

target_link_libraries(xunit_proof_test PRIVATE xblockstore xtxstore xblockmaker xcertauth xstatestore xdata xconfig xloader xgenesis xvledger xxbase ssl crypto secp256k1 gtest pthread)
