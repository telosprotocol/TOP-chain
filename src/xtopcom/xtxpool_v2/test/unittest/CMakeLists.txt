cmake_minimum_required(VERSION 3.8)

aux_source_directory(./ xtxpool_v2_test_src)

add_executable(xtxpool_v2_test ${xtxpool_v2_test_src})

#add_dependencies(xtxpool_v2_test xtxpool_v2 xblockstore xverifier xcertauth xstake xmutisig xxbase)

target_link_libraries(xtxpool_v2_test PRIVATE xtxpool_v2 xblockstore xverifier xcertauth xstake xmutisig ssl crypto xxbase secp256k1 xconfig xloader gtest pthread)

