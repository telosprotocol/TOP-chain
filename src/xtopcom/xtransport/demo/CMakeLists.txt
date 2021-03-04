aux_source_directory(./ xtransport_demo_dir)
add_executable(xtransport_demo ${xtransport_demo_dir})

#add_dependencies(xtransport_demo xgossip xtransport xxbase)
IF (CMAKE_SYSTEM_NAME MATCHES "Linux")
    target_link_libraries(xtransport_demo
        xtransport
        xpbase
        xcrypto
        xutility
        xxbase
        xdb
        protobuf
        -lgtest
        -lgmock
        -lpthread -ldl
        -lrt
    )
ELSEIF (CMAKE_SYSTEM_NAME MATCHES "Darwin")
    target_link_libraries(xtransport_demo
        xtransport
        xpbase
        xcrypto
        xutility
        xxbase
        xdb
        protobuf
        -lgtest
        -lgmock
        -lpthread -ldl
    )
ENDIF()
