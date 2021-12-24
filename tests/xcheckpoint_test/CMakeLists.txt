cmake_minimum_required(VERSION 3.8)

aux_source_directory(./ main)
aux_source_directory(./src src)
aux_source_directory(../../src/xtopcom/xblockstore/src/xcheckpoint.h checkpoint_header)
aux_source_directory(../../src/xtopcom/xblockstore/src/xvunithub.h blockstore_impl_header)
add_executable(xcheckpoint_test ${main} ${src} ${checkpoint_header} ${blockstore_impl_header})

#add_dependencies(xcheckpoint_test)

target_link_libraries(xcheckpoint_test PRIVATE xtxstore xblockstore xblockmaker xdata gtest)