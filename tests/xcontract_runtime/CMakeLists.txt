cmake_minimum_required(VERSION 3.8)
add_compile_options(-Wno-unused-parameter)
aux_source_directory(./src runtime_src)


add_executable(xcontract_runtime_test ${runtime_src})

add_dependencies(xcontract_runtime_test xcontract_runtime xvm  xloader xxbase xblockstore)

target_link_libraries(xcontract_runtime_test PRIVATE xcontract_runtime xvm lua xloader xxbase pthread jsoncpp gtest pthread xblockstore)
