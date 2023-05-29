aux_source_directory(./src xversion_src)
add_library(xversion STATIC ${xversion_src})
target_link_libraries(xversion PRIVATE xxbase)
