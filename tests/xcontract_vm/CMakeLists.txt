cmake_minimum_required(VERSION 3.8)
add_compile_options(-Wno-unused-parameter)

if (DISABLE_REAL_STAKE)
    add_definitions(-DXENABLE_MOCK_ZEC_STAKE)
endif()

aux_source_directory(./src xcontract_vm_src)

add_executable(xcontract_vm_test ${xcontract_vm_src})
add_dependencies(xcontract_vm_test xcontract_vm xcontract_runtime xvnode xvm  xloader xxbase xblockstore xgenesis)

target_link_libraries(xcontract_vm_test PRIVATE xcontract_vm xcontract_runtime xvnode xelection_lib xvm xloader xgenesis xxbase pthread jsoncpp gtest pthread xblockstore)
