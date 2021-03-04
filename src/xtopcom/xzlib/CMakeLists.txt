cmake_minimum_required(VERSION 3.8)

find_package(ZLIB REQUIRED)

aux_source_directory(./src xzlib_src)
add_library(xzlib STATIC ${xzlib_src})

#add_dependencies(xzlib xcodec)

target_include_directories(xzlib PUBLIC ${ZLIB_INCLUDE_DIRS})
target_link_libraries(xzlib PRIVATE xcodec ${ZLIB_LIBRARIES})

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")

    add_subdirectory(tests)
endif()
