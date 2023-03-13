cmake_minimum_required(VERSION 3.8)
add_compile_options(-Wno-unused-parameter)
aux_source_directory(./xcontract xcontract)
aux_source_directory(./xrec xrec)
aux_source_directory(./ xvm_test_src)
aux_source_directory(./xzec zec_src)

add_executable(xvm_test ${xvm_test_src} ${xcontract} ${xrec} ${zec_src})

#add_dependencies(xvm_test xvm xstore xloader xxbase xblockstore)

target_link_libraries(xvm_test PRIVATE xvm xstore xloader xxbase pthread jsoncpp gtest pthread xblockstore)
