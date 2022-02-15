cmake_minimum_required(VERSION 3.8)
add_compile_options(-std=c++11)

include_directories(SYSTEM src/xdepends/include)
if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    link_directories(src/xdepends/libs/linux)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    link_directories(src/xdepends/libs/mac)
endif()


aux_source_directory(./src src)
add_executable(grpc_cli ${src})

target_link_libraries(grpc_cli PRIVATE
    jsoncpp
    grpc++_unsecure grpc_unsecure gpr cares address_sorting z protobuf absl_synchronization
    -lpthread -ldl
)

