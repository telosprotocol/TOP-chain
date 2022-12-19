aux_source_directory(./src src)
add_executable(topio ${src})

add_dependencies(topio xpbase)
target_link_libraries(topio PRIVATE xsafebox xbasic xpbase jsoncpp pthread dl)
