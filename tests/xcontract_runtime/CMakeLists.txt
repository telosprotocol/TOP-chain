cmake_minimum_required(VERSION 3.8)
add_compile_options(-Wno-unused-parameter)
aux_source_directory(./src xcontract_runtime_src)
aux_source_directory(./erc20 erc20_src)
aux_source_directory(./system_contract  system_contract_src)
aux_source_directory(./property property_api_src)

if (BUILD_RUSTVM)
    add_executable(xcontract_runtime_test_erc20 ${erc20_src})
    add_dependencies(xcontract_runtime_test_erc20 xcontract_runtime xcontract_api xxbase)
    target_link_libraries(xcontract_runtime_test_erc20 PRIVATE xcontract_runtime xcontract_api xxbase pthread gtest)
endif()

add_executable(xcontract_runtime_test ${xcontract_runtime_src} ${system_contract_src} ${property_api_src})
add_dependencies(xcontract_runtime_test xcontract_vm xcontract_runtime xvnode xvm xgenesis xloader xxbase xblockstore)

target_link_libraries(xcontract_runtime_test PRIVATE xcontract_vm xcontract_runtime xvnode xelection_lib xvm xgenesis xloader xxbase pthread jsoncpp gtest pthread xblockstore)
