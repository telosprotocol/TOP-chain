cmake_minimum_required(VERSION 3.8)

aux_source_directory(./src xelection_src)

add_library(xelection STATIC ${xelection_src})

#add_dependencies(xelection xdata xcommon)
target_link_libraries(xelection PRIVATE xdata xcommon)
