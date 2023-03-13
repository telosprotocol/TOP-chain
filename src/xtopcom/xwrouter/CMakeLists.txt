aux_source_directory(./src xwrouter_src)
add_library(xwrouter STATIC ${xwrouter_src})

#add_dependencies(xwrouter xkad)

target_link_libraries(xwrouter PRIVATE xkad xgossip)


