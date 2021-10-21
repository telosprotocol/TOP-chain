cmake_minimum_required(VERSION 3.8)
add_compile_options(-Wno-unused-parameter)
aux_source_directory(./ xcontract_vm_src)

add_executable(xcontract_vm_test ${xcontract_vm_src})
add_dependencies(xcontract_vm_test xcontract_vm xcontract_runtime xvnode xvm  xloader xxbase xblockstore)

target_link_libraries(xcontract_vm_test PRIVATE xcontract_vm xcontract_runtime xvnode xelection_lib xvm lua xloader xxbase pthread jsoncpp gtest pthread xblockstore)
