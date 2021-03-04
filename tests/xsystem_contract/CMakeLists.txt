cmake_minimum_required(VERSION 3.8)

aux_source_directory(./ main_src)
aux_source_directory(./xelection_algorithm/fixture xelection_algorithm_fixture)
aux_source_directory(./xelection_algorithm/src xelection_algorithm)
aux_source_directory(./xelect_contract/src xelect_contract)
aux_source_directory(./xrec_standby_algorithm/fixture xrec_standby_algorithm_fixture)
aux_source_directory(./xrec_standby_algorithm/src xrec_standby_algorithm)
aux_source_directory(./xregistration/ xregistration)
aux_source_directory(./xreward/ xreward)

add_executable(xsystem_contract_test ${main_src} ${xelection_algorithm_fixture} ${xelection_algorithm} ${xelect_contract} ${xrec_standby_algorithm_fixture} ${xrec_standby_algorithm} ${xregistration} ${xreward})

#add_dependencies(xsystem_contract_test xvm xcommon xstore xblockstore xxbase xloader)
target_link_libraries(xsystem_contract_test PRIVATE xvm xcommon xstore xblockstore xxbase xloader gtest)
