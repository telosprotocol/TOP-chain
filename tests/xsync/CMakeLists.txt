aux_source_directory(./archive/ xsync_test_src)
add_executable(xsync_test ${xsync_test_src})

#add_dependencies(xmbus_test)
#target_link_libraries(xgenesis_test PRIVATE xloader xgenesis xtxstore xblockstore xstore xxbase xcommon gtest pthread jsoncpp)
#target_link_libraries(xunit_proof_test PRIVATE xvledger xblockstore xtxstore xblockmaker xcertauth xdata ssl crypto xxbase secp256k1 xconfig xloader xgenesis gtest pthread)
target_link_libraries(xsync_test PRIVATE xsync xmbus xvnetwork xdata xstore xblockstore gtest xtxstore xblockmaker xcertauth xdata pthread xloader xgenesis xtxstore)
