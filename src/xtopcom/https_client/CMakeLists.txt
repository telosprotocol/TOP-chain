cmake_minimum_required(VERSION 3.8)

aux_source_directory(./ https_client_src)
add_library(https_client ${https_client_src} )

#add_dependencies(https_client xpbase)
target_link_libraries(https_client PRIVATE ssl crypto)

