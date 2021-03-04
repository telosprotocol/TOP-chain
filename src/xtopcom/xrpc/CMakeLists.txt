cmake_minimum_required(VERSION 3.8)

aux_source_directory(./xerror error)
aux_source_directory(./prerequest prerequest)
aux_source_directory(./xedge edge)
aux_source_directory(./xcluster cluster)
aux_source_directory(./xgetblock getblock)
aux_source_directory(./xshard shard)
aux_source_directory(./ xrpc)
aux_source_directory(./xhttp xhttp)
aux_source_directory(./xws xws)
aux_source_directory(./xratelimit xratelimit)

add_library(xrpc STATIC ${error} ${prerequest} ${edge} ${cluster} ${getblock} ${shard} ${xrpc} ${xhttp} ${xws} ${xratelimit})

#add_dependencies(xrpc xconfig xcodec xvnetwork xcommon xrouter xverifier xxbase xxg)

target_link_libraries(xrpc PRIVATE xconfig msgpackc xcodec xvnetwork xcommon xrouter xverifier xbasic xxbase xxg crypto pthread)

if (BUILD_METRICS)
    #add_dependencies(xrpc xmetrics)
    target_link_libraries(xrpc PRIVATE xmetrics)
endif()


