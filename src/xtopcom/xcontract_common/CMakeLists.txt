add_compile_options(-Wpedantic)

if (DISABLE_REAL_STAKE)
    add_definitions(-DXENABLE_MOCK_ZEC_STAKE)
endif()

aux_source_directory(./src src_dir)
add_library(xcontract_common STATIC ${src_dir})

add_dependencies(xcontract_common xstate_accessor xdata xcommon xbasic xxbase)

target_link_libraries(xcontract_common PRIVATE xstate_accessor xdata xcommon xbasic xxbase pthread)
