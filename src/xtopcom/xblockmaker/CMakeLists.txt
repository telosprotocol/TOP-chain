#add_compile_options(-static -Wall -Werror)
add_compile_options(-static)

include_directories(.)
include_directories(./src)

aux_source_directory(./src src_dir)
add_library(xblockmaker STATIC ${src_dir})
IF (CMAKE_SYSTEM_NAME MATCHES "Darwin")
    target_link_libraries(xblockmaker PRIVATE
        xtxpool_v2
        xtxexecutor
        xblockstore
        xstore
        xverifier
        xvm
        xchain_upgrade
        xstake
        xxbase
        xcontract_vm
    )
ELSE()
    target_link_libraries(xblockmaker PRIVATE
        xtxpool_v2
        xtxexecutor
        xblockstore
        xstore
        xverifier
        xvm
        xchain_upgrade
        xstake
        xxbase
        intel_aes64
        xcontract_vm
    )
ENDIF()

if (BUILD_METRICS)
    target_link_libraries(xblockmaker PRIVATE xmetrics)
endif()

if (XENABLE_TESTS)
    enable_testing()
endif()
