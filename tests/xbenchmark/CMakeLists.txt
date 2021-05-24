cmake_minimum_required(VERSION 3.8)

aux_source_directory(./ benchmark_src)

add_executable(xbenchmark_test ${benchmark_src})

#add_dependencies(xsystem_contract_test xvm xcommon xstore xblockstore xxbase xloader)
target_link_libraries(xbenchmark_test PRIVATE xxbase gtest benchmark)
