cmake_minimum_required(VERSION 3.8)

add_compile_options(-static)

aux_source_directory(./ metrics_src)
aux_source_directory(./metrics_handler handler_src)

add_library(xmetrics STATIC ${metrics_src} ${handler_src})

target_link_libraries(xmetrics PRIVATE
    xxbase
)
