cmake_minimum_required(VERSION 3.8)

add_subdirectory(xgenerate_account)
# add_subdirectory(xtopcl_cli)

if (STATIC_CONSENSUS)
    add_subdirectory(xstatic_consensus_config_check)
endif()
