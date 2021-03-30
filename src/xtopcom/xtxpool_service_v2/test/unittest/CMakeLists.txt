cmake_minimum_required(VERSION 3.8)

aux_source_directory(./ xtxpool_svr_test_src)

add_executable(xtxpool_svr_test ${xtxpool_svr_test_src})

add_dependencies(xtxpool_svr_test xtxpoolsvr_v2 xtxpool_v2 xxbase xstore xverifier)

target_link_libraries(xtxpool_svr_test PRIVATE xtxpoolsvr_v2 xtxpool_v2 xxbase xstore xverifier gtest pthread)
