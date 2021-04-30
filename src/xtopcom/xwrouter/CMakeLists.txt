cmake_minimum_required(VERSION 3.8)

aux_source_directory(./src xwrouter_src)
add_library(xwrouter STATIC ${xwrouter_src})

#add_dependencies(xwrouter xkad)

target_link_libraries(xwrouter PRIVATE xkad xgossip cpp_redis tacopie)

# add_subdirectory(demo)

# if(XENABLE_TESTS)
#     add_subdirectory(tests)
# endif()
