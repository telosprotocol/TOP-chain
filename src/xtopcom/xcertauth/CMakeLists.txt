include_directories(.)
include_directories(./src)

aux_source_directory(./src src_dir)
add_library(xcertauth STATIC ${src_dir})

#add_dependencies(xcertauth xmutisig xxbase)

target_link_libraries(xcertauth PRIVATE xsafebox xmutisig xxbase)

if (BUILD_METRICS)
    target_link_libraries(xcertauth PRIVATE xmetrics)
endif()

