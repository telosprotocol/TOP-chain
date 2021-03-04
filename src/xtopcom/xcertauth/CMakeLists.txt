#add_compile_options(-static -Wall -Werror)
add_compile_options(-static)

include_directories(.)
include_directories(./src)

aux_source_directory(./src src_dir)
add_library(xcertauth STATIC ${src_dir})

#add_dependencies(xcertauth xmutisig xxbase)

target_link_libraries(xcertauth PRIVATE xmutisig xxbase)

# if (BUILD_METRICS)
#     #add_dependencies(xcertauth xmetrics)
#     target_link_libraries(xcertauth PRIVATE xmetrics)
# endif()

