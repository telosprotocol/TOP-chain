cmake_minimum_required(VERSION 3.8)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

project(XTOPCHAIN CXX C)

option(XENABLE_CODE_COVERAGE "Enable code coverage" OFF)
option(BUILD_METRICS "build metrics" OFF)
if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(BUILD_METRICS ON)
endif()
option(XCHAIN_FORKED_BY_DEFAULT "Enable chain forked logic" OFF)
option(ENABLE_METRICS_DATAOBJECT "Enable metrics dataobject" OFF)
option(DISABLE_CORE_SIGNAL_CAPTURE "disable core signal capture" OFF)
option(DISABLE_SIGNAL_CAPTURE "disable all signal capture" OFF)
option(XUSE_JEMALLOC "using jemalloc" OFF)

option(XBUILD_TEST "Enable building tests" OFF) # UNIT TEST disabled as default, use build option test to enable it.
if (XBUILD_TEST)
    message(STATUS "ENABLE  building tests ${XBUILD_TEST}")
endif()

option(DISABLE_EVM "Disable EVM" OFF) # EVM as default VM for now, use build option disable_evm to disable evm.
if (DISABLE_EVM)
    message(STATUS "DISABLE WITH EVM ${DISABLE_EVM}")
endif()

option(DISABLE_RATELIMIT "Disable rate limit" OFF) # RPC has rate limit as default, use build option noratelimit to disable it.
if (DISABLE_RATELIMIT)
    message(STATUS "DISABLE rate limit ${DISABLE_RATELIMIT}")
endif()

option(DISABLE_REAL_STAKE "Disable real stake require" OFF) # Real Stake requied as default, use build option mock_zec_stake to disable it.
if(DISABLE_REAL_STAKE)
    message(STATUS "DISABLE real stake ${DISABLE_REAL_STAKE}")
endif()

option(STATIC_CONSENSUS "Enable static consensus" OFF)
if (STATIC_CONSENSUS)
    if (NOT DISABLE_REAL_STAKE)
        message(FATAL_ERROR "should define mock_stake_zec before static_consensus")
    endif()
    message(STATUS "Enable static consensus ${STATIC_CONSENSUS}")
endif()

option(ELECT_WHEREAFTER "Enable static consensus, than continue election " OFF)
if (ELECT_WHEREAFTER)
    if (NOT STATIC_CONSENSUS)
        message(FATAL_ERROR "should define static_consensus before elect_whereafter")
    endif()
    message(STATUS "Enable static consensus, than continue election ${ELECT_WHEREAFTER}")
endif()

option(CONSENSUS_SWAP "Enable static consensus, than swap consensus group nodes" OFF)
if (CONSENSUS_SWAP)
    if (NOT ELECT_WHEREAFTER)
        message(FATAL_ERROR "should define elect_whereafter before consensus_swap")
    endif()
    message(STATUS "Enable static consensus, than swap consensus group nodes${CONSENSUS_SWAP}")
endif()

option(MAINNET_ACTIVATED "Mainnet activated" OFF) # Mainnet Activated if off as default, use build option mainnet_activated to enable it.
if (MAINNET_ACTIVATED)
    message(STATUS "Mainnet Activated ${MAINNET_ACTIVATED}")
endif()

set(CMAKE_VERBOSE_MAKEFILE OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (NOT CMAKE_CXX_COMPILER)
    message(FATAL_ERROR "C++ Compiler not found")
endif()

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

string(TOLOWER ${CMAKE_BUILD_TYPE} XBUILD_TYPE)

#=============================================================================
# for git version
#==============================================================================
include(cmake/gitinfo.cmake)
get_git_info()

include(cmake/version.cmake)

add_definitions(
    -DBUILD_USER_LIB # for -lcommon
    -D__TRACKING_PBFT_PROCESS__
    -DTEST_FOR_ELECTION
    -DGOSSIP_WITH_WROUTER
)

#==============================================================================
# In order to set default executable and library binaries' location,
# one solution, by setting
# variables (CMAKE_RUNTIME_OUTPUT_DIRECTORY / CMAKE_LIBRARY_OUTPUT_DIRECTORY)
# or properties (RUNTIME_OUTPUT_DIRECTORY / LIBRARY_OUTPUT_DIRECTORY),
# doesn't work, althrough CMake documentation says it's the recommonded way.
# Fall back to use EXECUTABLE_OUTPUT_PATH / LIBRARY_OUTPUT_PATH.
#==============================================================================
#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
#set_property(DIRECTORY PROPERTY RUNTIME_OUTPUT_DIRECTORY ${XTOPCHAIN_BINARY_DIR}/bin/${CMAKE_BUILD_TYPE})
#set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR/lib})
#set_property(DIRECTORY PROPERTY LIBRARY_OUTPUT_DIRECTORY ${XTOPCHAIN_BINARY_DIR}/lib/${CMAKE_BUILD_TYPE})

set(EXECUTABLE_OUTPUT_PATH ${XTOPCHAIN_BINARY_DIR}/bin/${CMAKE_SYSTEM_NAME}/)
set(LIBRARY_OUTPUT_PATH ${XTOPCHAIN_BINARY_DIR}/lib/${CMAKE_SYSTEM_NAME}/)

if (${CMAKE_CXX_COMPILER_ID} STREQUAL GNU)
    # common compiling options
    add_compile_options(
        -fno-strict-aliasing

        # -fthreadsafe-statics
        -pthread
        -fstack-protector-strong
        -fno-short-enums
        -fPIC
    )

    # CXX only
    add_compile_options($<$<STREQUAL:$<TARGET_PROPERTY:LINKER_LANGUAGE>,CXX>:-frtti>)
    add_compile_options($<$<STREQUAL:$<TARGET_PROPERTY:LINKER_LANGUAGE>,CXX>:-fexceptions>)
    add_compile_options($<$<STREQUAL:$<TARGET_PROPERTY:LINKER_LANGUAGE>,CXX>:-fthreadsafe-statics>)

    add_definitions(-D_GNU_SOURCE)

    if (${CMAKE_CXX_COMPILER_ID} STREQUAL Clang OR ${CMAKE_CXX_COMPILER_ID} STREQUAL AppleClang)
        # add_compile_options(-Wabstract-final-class -Wshift-overflow -Wint-to-void-pointer-cast)
        add_compile_options(-Werror=self-assign
                            -Werror=exceptions)
    endif()

    #==============================================================================
    # Althrough CMake documentation (3.6.3) says add_compile_options supports
    # generator expressions, the actual result is -buggy-.
    # add_compile_options doesn't fully working with generator expressions.
    #
    # $<$<OR:?1,?2>:...>
    # for OR expression, do not insert any whitespace character between ?1 and ?2.
    #==============================================================================
    # options for Debug
    #add_compile_options($<$<OR:$<CONFIG:Debug>,$<CONFIG:debug>>:-g2 -ggdb -O0 -fno-omit-frame-pointer>)
    set_property(DIRECTORY APPEND PROPERTY COMPILE_OPTIONS $<$<OR:$<CONFIG:Debug>,$<CONFIG:debug>>:-g3 -ggdb -O0 -fno-omit-frame-pointer>)
    set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS $<$<OR:$<CONFIG:Debug>,$<CONFIG:debug>>:DEBUG _DEBUG>)

    # options for Release & RelWithDebInfo
    # add_compile_options($<$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>,$<CONFIG:release>>:-fomit-frame-pointer -ffunction-sections -fdata-sections>)
    set_property(DIRECTORY APPEND PROPERTY COMPILE_OPTIONS $<$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>,$<CONFIG:release>>:-ffunction-sections -fdata-sections>)
    set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS $<$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>,$<CONFIG:release>>:NDEBUG>)

    # options for Release
    add_compile_options($<$<OR:$<CONFIG:Release>,$<CONFIG:release>>:-g0>)

    # options for RelWithDebInfo
    add_compile_options($<$<CONFIG:RelWithDebInfo>:-g1>)
    add_compile_options($<$<CONFIG:RelWithDebInfo>:-ggdb>)
else()
    message(FATAL_ERROR "Not supported C++ Compiler: " ${CMAKE_CXX_COMPILER_ID})
endif()

if ((BUILD_GPERF OR BUILD_GHPERF OR TCMALLOC) AND XUSE_JEMALLOC)
    message(FATAL_ERROR "Not allowed to enable tcmalloc and jemalloc at the same time")
endif()

if (ADDRESS_SANITIZER)
    add_compile_options(-fsanitize=address)
endif()

if (BUILD_METRICS)
    add_definitions(-DENABLE_METRICS)
endif()

if (BUILD_XSECURITY)
    add_definitions(-DENABLE_XSECURITY)
endif()

if (BUILD_GPERF)
    add_definitions(-DENABLE_GPERF)
endif()

if (BUILD_GHPERF)
    add_definitions(-DENABLE_GHPERF)
endif()

if (TCMALLOC)
    add_definitions(-DENABLE_TCMALLOC)
endif()

# for slash test
if (SLASH_TEST)
    add_definitions(-DSLASH_TEST)
endif()

# for workload test
if (WORKLOAD_TEST)
    add_definitions(-DWORKLOAD_TEST)
endif()

if (XENABLE_PSTACK)
    add_definitions(-DXENABLE_PSTACK)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -rdynamic")
endif()

if (XENABLE_MEMCHECK_DBG)
    add_definitions(-DXENABLE_MEMCHECK_DBG)
endif()

if (XENABLE_P2P_BENDWIDTH)
    add_definitions(-DXENABLE_P2P_BENDWIDTH)
endif()

if (NO_TX_BATCH)
    add_definitions(-DNO_TX_BATCH)
endif()

if (ENABLE_CREATE_USER)
    add_definitions(-DENABLE_CREATE_USER)
endif()

if (DB_KV_STATISTIC)
    add_definitions(-DDB_KV_STATISTIC)
endif()

if (DB_CACHE)
    add_definitions(-DDB_CACHE)
endif()

if (XBUILD_CI)
    add_definitions(-DXBUILD_CI)
endif()

if (XBUILD_DEV)
    add_definitions(-DXBUILD_DEV)
endif()

if (XBUILD_GALILEO)
    add_definitions(-DXBUILD_GALILEO)
endif()

if (XBUILD_BOUNTY)
    add_definitions(-DXBUILD_BOUNTY)
endif()

if (BUILD_RUSTVM)
    add_definitions(-DBUILD_RUSTVM)
endif()

if (LEAK_TRACER)
    add_definitions(-DLEAK_TRACER)
endif()

if (STORE_UNIT_BLOCK)
    add_definitions(-DSTORE_UNIT_BLOCK)
endif()

if (XCHAIN_FORKED_BY_DEFAULT)
    add_definitions(-DXCHAIN_FORKED_BY_DEFAULT=${XCHAIN_FORKED_BY_DEFAULT})
    message("XCHAIN_FORKED_BY_DEFAULT: ${XCHAIN_FORKED_BY_DEFAULT}")
endif()

if (ENABLE_METRICS_DATAOBJECT)
    if (NOT BUILD_METRICS)
        message(FATAL_ERROR "should define metrics before metrics_dataobject")
    endif()
    add_definitions(-DENABLE_METRICS_DATAOBJECT)
endif()

if (DISABLE_CORE_SIGNAL_CAPTURE)
    add_definitions(-DDISABLE_CORE_SIGNAL_CAPTURE)
endif()

if (DISABLE_SIGNAL_CAPTURE)
    add_definitions(-DDISABLE_SIGNAL_CAPTURE)
endif()

if (${CMAKE_BUILD_TYPE} STREQUAL RelWithDebInfo)
    add_definitions(-DRELEASEDEBINFO)
endif()

if (CHECKPOINT_TEST)
    add_definitions(-DCHECKPOINT_TEST)
endif()

if (ETH_BRIDGE_TEST)
    add_definitions(-DETH_BRIDGE_TEST)
endif()

if (ETH2_SEPOLIA)
    add_definitions(-DETH2_SEPOLIA)
endif()

if (CACHE_SIZE_STATISTIC)
    add_definitions(-DCACHE_SIZE_STATISTIC)
endif()

if (PERIOD_MOCK)
    add_definitions(-DPERIOD_MOCK)
endif()

if (CROSS_TX_DBG)
    add_definitions(-DCROSS_TX_DBG)
endif()

if (${CMAKE_SYSTEM_NAME} STREQUAL Linux OR ${CMAKE_SYSTEM_NAME} STREQUAL Darwin)
#  find_package(PkgConfig REQUIRED)

endif()

message(STATUS "CMAKE_BUILD_TYPE:" ${CMAKE_BUILD_TYPE})
message(STATUS "CMAKE_SYSTEM_NAME:" ${CMAKE_SYSTEM_NAME})
message(STATUS "CMAKE_CXX_COMPILER_ID:" ${CMAKE_CXX_COMPILER_ID})
message(STATUS "XENABLE_CODE_COVERAGE:" ${XENABLE_CODE_COVERAGE})
message(STATUS "XBUILD_TEST:" ${XBUILD_TEST})
message(STATUS "BUILD_METRICS:" ${BUILD_METRICS})
message(STATUS "ADDRESS_SANITIZER:" ${ADDRESS_SANITIZER})
message(STATUS "XCHAIN_FORKED_BY_DEFAULT:" ${XCHAIN_FORKED_BY_DEFAULT})
message(STATUS "DISABLE_CORE_SIGNAL_CAPTURE:" ${DISABLE_CORE_SIGNAL_CAPTURE})
message(STATUS "DISABLE_SIGNAL_CAPTURE:" ${DISABLE_SIGNAL_CAPTURE})
message(STATUS "XUSE_JEMALLOC:" ${XUSE_JEMALLOC})

find_package(Threads REQUIRED)
if (XUSE_JEMALLOC)
    link_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/xtopcom/xdepends/jemalloc/lib)
    link_libraries(jemalloc_pic)
elseif(BUILD_GPERF OR BUILD_GHPERF OR TCMALLOC)
    include_directories(SYSTEM src/xtopcom/xdepends/gperftools/src)
    link_directories(src/xtopcom/xdepends/gperftools/.libs)
endif()

include_directories(src/xtopcom)  # xtopcom build warning will not print
include_directories(SYSTEM src/xtopcom/third_party)
include_directories(SYSTEM src/xtopcom/xdepends/include/trezor-crypto)
include_directories(SYSTEM src/xtopcom/xdepends/include)
include_directories(SYSTEM src/xtopcom/xdepends/GSL/include)
include_directories(src/xtopcom/xtopcl/src)  # xtopcom build warning will not print
include_directories(src/xtopcom/xtopcl/include)  # xtopcom build warning will not print

include_directories(src/libraries)
include_directories(src/services)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

link_directories(${LIBRARY_OUTPUT_PATH})
link_directories(libs/Linux/${XBUILD_TYPE})

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_compile_options(
        # -Wl,-z,relro
        # -Wl,-z,noexecstack
        # -Wl,--no-undefined
        # -Wl,-z,now
        -Wno-unused-function
        -Wno-unused-value
        -Wno-unused-variable
    )
    include_directories(SYSTEM src/xtopcom/xdepends/include_linux)
    # link_directories(src/externals/libs/${CMAKE_SYSTEM_NAME}/)
    IF (CMAKE_BUILD_TYPE STREQUAL Debug)
        link_directories(src/xtopcom/xbase/libs/${CMAKE_SYSTEM_NAME}/${XBUILD_TYPE})
        link_directories(src/xtopcom/libs/${CMAKE_SYSTEM_NAME}/${XBUILD_TYPE})
        link_directories(src/xtopcom/xdepends/libs/linux)
    ELSE()
        link_directories(src/xtopcom/xbase/libs/${CMAKE_SYSTEM_NAME}/release)
        link_directories(src/xtopcom/libs/${CMAKE_SYSTEM_NAME}/release)
        link_directories(src/xtopcom/xdepends/libs/linux)
    ENDIF()
    
    add_definitions(-D_GLIBCXX_USE_CXX11_ABI=0)

    set(XDB_TYPE "ROCKSDB")
    add_subdirectory(src)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    add_compile_options(
        -Wno-unused-private-field
        -Wno-inconsistent-missing-override
        # -Wno-constexpr-not-const
        -Wno-c++14-extensions
        -Wno-unused-parameter
        -Wno-sign-conversion
        -Wno-delete-non-virtual-dtor
        -Wno-sign-compare
        -Wno-conversion
        # -Wno-missing-declarations
        # -Wno-conversion
        # -Wno-float-conversion
        -Wno-unused-variable
        -Wno-reorder
        -Wno-pragma-once-outside-header
        -Wno-shadow
        # -Wno-tautological-constant-out-of-range-compare
        # -Wno-shift-op-parentheses
        -Wno-unreachable-code
        -Wno-pessimizing-move
        -Wno-gnu-zero-variadic-macro-arguments
        -Wno-return-type
        -Wno-unused-function
        -Wno-shorten-64-to-32
        -Wno-unused-value
        -Wno-c++11-narrowing
        -Wno-unused-label
        -Wno-unused-lambda-capture
        -Wno-defaulted-function-deleted
        -Wno-overloaded-virtual
        -Wno-sometimes-uninitialized

    )

    # for xg
    find_library(CFLIB CoreFoundation)
    add_definitions(-DGUID_CFUUID)
    add_definitions(-DXBUILD_DEV)
    # message("CFLIB: ${CFLIB}")

    # link_directories(src/externals/libs/${CMAKE_SYSTEM_NAME}/)
    IF (CMAKE_BUILD_TYPE STREQUAL Debug)
        link_directories(src/xtopcom/xbase/libs/MacOS/${XBUILD_TYPE})
    ELSE()
        link_directories(src/xtopcom/xbase/libs/MacOS/release)
    ENDIF()
    link_directories(src/xtopcom/xdepends/libs/mac)

    set(XDB_TYPE "ROCKSDB")
    add_subdirectory(src)
endif()

if (XBUILD_TEST)
    file(STRINGS /etc/os-release distro REGEX "^NAME=")
    string(REGEX REPLACE "NAME=\"(.*)\"" "\\1" distro "${distro}")
    message("found ${distro}")
    if (${distro} STREQUAL "Ubuntu")
        message("set no pie on ${distro} for test")
        # todo update libgtest to avoid this
        set(CMAKE_CXX_FLAGS "-no-pie")
    endif()
    enable_testing()
    
    include_directories(SYSTEM src/xtopcom/xdepends/googletest/googletest/include)
    include_directories(SYSTEM src/xtopcom/xdepends/googletest/googlemock/include)
    include_directories(.)

    add_subdirectory(tests)

endif()

# Users don't have to configure CMAKE_INSTALL_PREFIX unless they want to customize the destination.
if (CMAKE_INSTALL_PREFIX STREQUAL "/usr/local")
    #set(CMAKE_INSTALL_PREFIX "/usr/local/topio" CACHE STRING "" FORCE)
endif()
#message (STATUS "TOPIO Installation directory: CMAKE_INSTALL_PREFIX = " ${CMAKE_INSTALL_PREFIX})

set(INSTALL_BIN_PATH /usr/bin/)
set(INSTALL_LIB_PATH /lib/)

INSTALL(TARGETS topio xtopchain
RUNTIME DESTINATION ${INSTALL_BIN_PATH}
LIBRARY DESTINATION ${INSTALL_LIB_PATH}
ARCHIVE DESTINATION ${INSTALL_LIB_PATH}
)
