add_compile_options(-static -Wall -Werror)

aux_source_directory(./src src_dir)
add_library(xdata STATIC ${src_dir})

add_dependencies(xdata xconfig xutility xcommon xcrypto xbasic xxbase)

target_link_libraries(xdata PRIVATE xconfig xutility xcommon xcrypto xchain_fork xbasic xvledger jsoncpp xxbase pthread)

if (BUILD_METRICS)
    add_dependencies(xdata xmetrics)
    target_link_libraries(xdata PRIVATE xmetrics)
endif()
