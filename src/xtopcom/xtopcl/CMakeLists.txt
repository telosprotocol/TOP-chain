add_compile_options(-static -Wall -Werror)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# compile definitions
add_definitions(
    -DUSE_STANDALONE_ASIO
    -DASIO_STANDALONE
)

if( CMAKE_BUILD_TYPE STREQUAL "Debug" )
    add_definitions(-DLOG_CONSOLE)
    if (NOT MSVC)
        add_compile_options("-g")
    endif()
endif()

set(PROJ_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(SRC_DIR ${PROJ_DIR}/src)
aux_source_directory(${SRC_DIR} cppsdk_src)
aux_source_directory(${SRC_DIR}/base cppsdk_src)
aux_source_directory(${SRC_DIR}/network cppsdk_src)
aux_source_directory(${SRC_DIR}/task cppsdk_src)

#compile definitions
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_compile_options(-Wno-deprecated-declarations)
    add_compile_options(-Wno-write-strings)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    add_compile_options(-Wno-writable-strings)
    add_compile_options(-Wno-deprecated-declarations)
    add_compile_options(-Wno-write-strings)
    add_compile_options(-Wno-unused-variable)
    add_compile_options(-Wno-unused-function)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    add_definitions(-D_WIN32_WINNT=0x0501)
    add_definitions(-DNOMINMAX)  # enable std::min/std::max
    add_definitions(-D_CRT_SECURE_NO_WARNINGS) # disable strncpy -> strncpy_s warnings
    add_compile_options(/Zc:__cplusplus) # using correct __cplusplus value
    add_compile_options(/utf-8) # input/output as utf-8
endif()

set(LINK_ARGS secp256k1 trezor-crypto ssl crypto cryptopp)

if(NOT MSVC)
    set(LINK_ARGS ${LINK_ARGS} pthread)
endif()

# SET TARGET
add_library(xtopcl STATIC ${cppsdk_src})
#add_dependencies(xtopcl xxbase xdata xcrypto)
target_include_directories(xtopcl  PUBLIC   ${PROJ_DIR}/include
                                            ${PROJ_DIR}/include/base
                                            ${PROJ_DIR}/include/network
                                            ${PROJ_DIR}/include/task
                                            ${PROJ_DIR}/include/web
                                            ${PROJ_DIR}/include/xtop
)
target_link_libraries(xtopcl PRIVATE ${LINK_ARGS} xxbase xdata xelect_net jsoncpp)

