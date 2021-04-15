add_compile_options(-Wall -Werror -Wpedantic)

aux_source_directory(./src src_dir)
add_library(xcontract_runtime STATIC ${src_dir})

add_dependencies(xcontract_runtime xcontract_common xdata xcommon xbasic xxbase)

target_link_libraries(xcontract_runtime PRIVATE xcontract_common xdata xcommon xbasic xxbase pthread)

if (XENABLE_TESTS)
    add_subdirectory(xuser/test)
endif()