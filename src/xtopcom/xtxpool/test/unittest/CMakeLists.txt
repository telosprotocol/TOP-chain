cmake_minimum_required(VERSION 3.8)

aux_source_directory(./ xtxpool_test_src)

add_executable(xtxpool_test ${xtxpool_test_src})

#add_dependencies(xtxpool_test xtxpool xblockstore xverifier xcertauth xstake xmutisig xxbase)

target_link_libraries(xtxpool_test PRIVATE xtxpool xblockstore xverifier xcertauth xstake xmutisig ssl crypto xxbase secp256k1 xconfig xloader gtest pthread)
