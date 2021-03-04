aux_source_directory(./ xwrouter_demo_dir)
add_executable(xwrouter_demo ${xwrouter_demo_dir})

add_definitions(
)

#add_dependencies(xwrouter_demo xvnetwork xxbase xpbase)

IF (CMAKE_SYSTEM_NAME MATCHES "Linux")
    target_link_libraries(xwrouter_demo
        xwrouter
        xvnetwork
        xgossip
        xkad
        xtransport
        xpbase
        xcrypto
        xutility
        xxbase
        protobuf
        -lgtest
        -lgmock
        -lpthread -ldl
        -lrt
    )
ELSEIF (CMAKE_SYSTEM_NAME MATCHES "Darwin")
    target_link_libraries(xwrouter_demo
        xwrouter
        xvnetwork
        xgossip
        xkad
        xtransport
        xpbase
        xcrypto
        xutility
        xxbase
        protobuf
        -lgtest
        -lgmock
        -lpthread -ldl
    )
ENDIF()
