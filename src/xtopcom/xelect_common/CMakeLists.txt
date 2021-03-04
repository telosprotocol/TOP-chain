cmake_minimum_required(VERSION 3.8)

aux_source_directory(./src SRCS)
aux_source_directory(./proto SRCS)
add_library(xelect_common STATIC ${SRCS})

#add_dependencies(xelect_common xpbase)
target_link_libraries(xelect_common PRIVATE protobuf xpbase)

# if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
#     target_link_libraries(xelect_common PRIVATE protobuf)
#     if (XENABLE_TESTS and XENABLE_CODE_COVERAGE)
#         add_executable(xstaticec_test test/main.cc)
#         target_link_libraries(xstaticec_test xstaticec gcov)
#     endif()
# endif()
