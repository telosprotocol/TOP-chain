cmake_minimum_required(VERSION 3.8)

#set(XTOPCHAIN_VERSION_FEATURES "0")
#set(XTOPCHAIN_VERSION_MAJOR "0")
#set(XTOPCHAIN_VERSION_MINOR "0")
#set(XTOPCHAIN_VERSION_PATCH "0")
#set(XTOPCHAIN_VERSION "${XTOPCHAIN_VERSION_FEATURES}.${XTOPCHAIN_VERSION_MAJOR}.${XTOPCHAIN_VERSION_MINOR}.${XTOPCHAIN_VERSION_PATCH}")
#
#add_definitions(-DXTOPCHAIN_VERSION="${XTOPCHAIN_VERSION}")


#compile definitions
add_definitions(
    -DUSE_STANDALONE_ASIO
    -DASIO_STANDALONE
    # -D__STRICT_ANSI__
    -DRPC_BUILD
)
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

set(LINK_ARGS secp256k1 trezor-crypto ssl crypto xchain_upgrade)
if(NOT MSVC)
    set(LINK_ARGS ${LINK_ARGS} hiredis pthread)
endif()

add_executable(xtopchain_bin main.cpp)
set_target_properties(xtopchain_bin
    PROPERTIES OUTPUT_NAME "xtopchain"
)
if (BUILD_GPERF)
    target_link_libraries(xtopchain_bin PRIVATE xchaininit tcmalloc_and_profiler)
elseif(TCMALLOC)
    target_link_libraries(xtopchain_bin PRIVATE xchaininit tcmalloc_minimal)
else()
    target_link_libraries(xtopchain_bin PRIVATE xchaininit)
endif()
add_library(xtopchain SHARED so.cpp)
set_target_properties(xtopchain PROPERTIES
    VERSION   "${PROGRAM_VERSION}"
    SOVERSION "${PROGRAM_VERSION}"
)
# target_link_libraries(xtopchain PRIVATE xchaininit ${LINK_ARGS} xtopcl xxbase xdata xcrypto jsoncpp)
target_link_libraries(xtopchain PRIVATE xchaininit ${LINK_ARGS} xtopcl
    xelect_net  xwrouter xkad xpbase xgossip xtransport xcommon xnetwork xbasic https_client
    xxbase xdata xcrypto jsoncpp)
target_link_libraries(xtopchain_bin PRIVATE xchaininit ${LINK_ARGS} xtopcl
    xelect_net xwrouter xkad xpbase xgossip xtransport xcommon xnetwork xbasic https_client
    xxbase xdata xcrypto jsoncpp)

if (BUILD_METRICS)
    #add_dependencies(xtopchain xmetrics)
    target_link_libraries(xtopchain PRIVATE xmetrics)
    #add_dependencies(xtopchain_bin xmetrics)
    target_link_libraries(xtopchain_bin PRIVATE xmetrics)
endif()
