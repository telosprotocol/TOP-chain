add_compile_options(-Wall -Werror -Wpedantic)

aux_source_directory(./src src_dir)
add_library(xcontract_runtime STATIC ${src_dir})

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
enable_language(Rust)
include(CMakeCargo)
add_subdirectory(xwasm_vm)

add_dependencies(xcontract_runtime xcontract_common xdata xcommon xbasic xxbase xwasm_vm)

target_link_libraries(xcontract_runtime PRIVATE xcontract_common xdata xcommon xbasic xxbase xwasm_vm pthread dl)

if (XENABLE_TESTS)
    add_subdirectory(xuser/test)
endif()