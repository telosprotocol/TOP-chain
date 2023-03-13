add_definitions(-DUSE_BACKTRACE)
include_directories(.)

aux_source_directory(./src leaktracer_src)
add_library(leaktracer STATIC ${leaktracer_src})