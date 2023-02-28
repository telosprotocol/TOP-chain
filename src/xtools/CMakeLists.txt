add_subdirectory(xgenerate_account)
add_subdirectory(xdecoders)
add_subdirectory(xdb_export)

if (STATIC_CONSENSUS)
    add_subdirectory(xstatic_consensus_config_check)
endif()

if (DB_TOOL)
    add_subdirectory(xdb_tool)
endif()
