
if (DISABLE_REAL_STAKE)
add_definitions(-DXENABLE_MOCK_ZEC_STAKE)
endif()

aux_source_directory(./ main_src)
aux_source_directory(./xelection_algorithm/fixture xelection_algorithm_fixture)
aux_source_directory(./xelection_algorithm/src xelection_algorithm)
aux_source_directory(./xelect_contract/src xelect_contract)
aux_source_directory(./xrec_standby_algorithm/fixture xrec_standby_algorithm_fixture)
aux_source_directory(./xrec_standby_algorithm/src xrec_standby_algorithm)
aux_source_directory(./xregistration/ xregistration)
aux_source_directory(./xreward_algorithm/ xreward_algorithm)
aux_source_directory(./xreward/ xreward)
aux_source_directory(./xslash/ xslash)
aux_source_directory(./xrec_proposal rec_proposal)
aux_source_directory(./xtable_statistic xtable_statistic)
aux_source_directory(./xrelay_contract xrelay_contract)

add_executable(xsystem_contract_test ${main_src} ${xelection_algorithm_fixture} ${xelection_algorithm} ${xelect_contract} ${xreward_algorithm} ${xrec_standby_algorithm_fixture} ${xrec_standby_algorithm} ${xreward} ${xslash} ${rec_proposal} ${xtable_statistic} ${xregistration} ${xrelay_contract})

add_dependencies(xsystem_contract_test xvledger xvm xcommon xstore xblockstore xtxstore xxbase xloader xgenesis)
target_link_libraries(xsystem_contract_test PRIVATE xvledger xvm xvledger xcommon xstore xblockstore xtxstore xblockmaker xelection_lib xxbase xloader xgenesis gtest)

if (XBUILD_CONSORTIUM)
    aux_source_directory(./xnode_manage_contract xnode_manage_contract)
    target_sources(xsystem_contract_test PRIVATE ${xnode_manage_contract})
endif()



