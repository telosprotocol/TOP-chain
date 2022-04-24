cmake_minimum_required(VERSION 3.8)
add_compile_options(-static -Wall -Werror)

aux_source_directory(./ xvm)
aux_source_directory(./manager manager)
aux_source_directory(./xcontract xcontract)
aux_source_directory(./xserialization xserialization)
aux_source_directory(./xerror xerror)
aux_source_directory(./xsystem_contracts/deploy system_contracts_deploy)
aux_source_directory(./xsystem_contracts/src system_contracts_src)
aux_source_directory(./xsystem_contracts/xeth system_contracts_xeth)

add_library(xvm STATIC ${xvm} ${manager} ${xcontract} ${xserialization} ${xerror} ${system_contracts_deploy} ${system_contracts_src} ${system_contracts_xeth})

add_dependencies(xvm xconfig xstore xverifier xdata xcommon xcodec xbasic xxbase xcertauth xchain_upgrade)

target_link_libraries(xvm PRIVATE xconfig xevm_common xrouter xverifier xdata xcommon xcodec xbasic xstore xxbase protobuf lua xcertauth xchain_upgrade ethash keccak)

if (BUILD_METRICS)
    #add_dependencies(xvm xmetrics)
    target_link_libraries(xvm PRIVATE xmetrics)
endif()
