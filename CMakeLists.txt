cmake_minimum_required(VERSION 3.8)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

project(XTOPCHAIN CXX C)

option(XENABLE_CODE_COVERAGE "Enable code coverage" OFF)
option(XENABLE_TESTS "Enable building tests" OFF)
option(XENABLE_VERBOSE_DBG "Enable verbose dbg" OFF)
option(XENABLE_EXCEPTION_AS_ERROR "Treat exception as xerror in debug mode" OFF)
option(BUILD_METRICS "build metrics" OFF)
if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(BUILD_METRICS ON)
endif()
option(XDISABLE_RATELIMIT "Disable rate limit" OFF)
option(XENABLE_CONFIG_CHECK "Enable config check when node boot" OFF)
option(XENABLE_MOCK_ZEC_STAKE "Enable mocking stake for ZEC" OFF)
option(XCHAIN_FORKED_BY_DEFAULT "Enable chain forked logic" OFF)
option(ENABLE_METRICS_DATAOBJECT "Enable metrics dataobject" OFF)
option(DISABLE_CORE_SIGNAL_CAPTURE "disable core signal capture" OFF)

if (XENABLE_TESTS)
    set(XENABLE_BLOCK_VERIFY OFF)
    set(XSYNC_TEST ON)
else()
    set(XENABLE_BLOCK_VERIFY OFF)
    set(XSYNC_TEST OFF)
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

if (${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
    if (NOT CMAKE_TOOLCHAIN_FILE)
        message(FATAL_ERROR "CMAKE_TOOLCHAIN_FILE property not defined. Set it in CMakeSettings.json file.")
    endif()

    if (NOT EXISTS ${CMAKE_TOOLCHAIN_FILE})
        message(FATAL_ERROR "CMAKE_TOOLCHAIN_FILE not found.")
    endif()
endif()

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

if (${CMAKE_CXX_COMPILER_ID} STREQUAL Clang OR ${CMAKE_CXX_COMPILER_ID} STREQUAL AppleClang OR ${CMAKE_CXX_COMPILER_ID} STREQUAL GNU)
    # set common options for Clang or GNUCXX

    add_compile_options(
        $<$<STREQUAL:$<TARGET_PROPERTY:LINKER_LANGUAGE>,CXX>:-fexceptions>  # https://gcc.gnu.org/onlinedocs/gcc-7.3.0/gcc/Code-Gen-Options.html#index-fexceptions
        $<$<STREQUAL:$<TARGET_PROPERTY:LINKER_LANGUAGE>,CXX>:-frtti>        # https://gcc.gnu.org/onlinedocs/gcc-7.3.0/gcc/C_002b_002b-Dialect-Options.html#index-fno-rtti
    )

    # common compiling options
    add_compile_options(
        -fno-strict-aliasing
        -fthreadsafe-statics
        -pthread
        -fstack-protector-strong
        -fno-short-enums
        -fPIC
    )

    if (ADDRESS_SANITIZER)
        add_compile_options(-fsanitize=address)
    endif()

    # set warnings
    add_compile_options(
        -Wall               # https://gcc.gnu.org/onlinedocs/gcc-4.8.5/gcc/Warning-Options.html#index-Wall-263
        -Werror             # https://gcc.gnu.org/onlinedocs/gcc-4.8.5/gcc/Warning-Options.html#index-Werror-254
#        -Wextra             # https://gcc.gnu.org/onlinedocs/gcc-4.8.5/gcc/Warning-Options.html#index-Wextra-266
#        -Wpedantic          # https://gcc.gnu.org/onlinedocs/gcc-4.8.5/gcc/Warning-Options.html#index-Wpedantic-261
#        -Wconversion        # https://gcc.gnu.org/onlinedocs/gcc-4.8.5/gcc/Warning-Options.html#index-Wconversion-426
#        -Wempty-body
        -Wfatal-errors
#        -Wshadow
#        -Wzero-as-null-pointer-constant
        -Wunreachable-code
        -Wno-missing-field-initializers
        -Werror=return-type
        -Werror=write-strings
        -Werror=unused-label
        -Werror=switch
    )

    add_definitions(-D_GNU_SOURCE)

    if (${CMAKE_CXX_COMPILER_ID} STREQUAL Clang OR ${CMAKE_CXX_COMPILER_ID} STREQUAL AppleClang)
        # add_compile_options(-Wabstract-final-class -Wshift-overflow -Wint-to-void-pointer-cast)
        add_compile_options(-Werror=self-assign
                            -Werror=exceptions)
    endif()

    if (${CMAKE_CXX_COMPILER_ID} STREQUAL GNU)
        # add_compile_options(-Wsuggest-final-methods -Wsuggest-final-types -Wsuggest-override)
        # add_compile_options(-fno-builtin-malloc -fno-builtin-calloc -fno-builtin-realloc -fno-builtin-free)  // for tcmalloc
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

elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)

    # common compile options
    add_compile_options(
        /Zc:auto,forScope,inline,wchar_t
        /Zc:externConstexpr   # https://docs.microsoft.com/en-us/cpp/build/reference/zc-externconstexpr
        /Zc:implicitNoexcept  # https://docs.microsoft.com/en-us/cpp/build/reference/zc-implicitnoexcept-implicit-exception-specifiers
        /Zc:noexceptTypes     # https://docs.microsoft.com/en-us/cpp/build/reference/zc-noexcepttypes
        /Zc:referenceBinding  # https://docs.microsoft.com/en-us/cpp/build/reference/zc-referencebinding-enforce-reference-binding-rules
        /Zc:rvalueCast        # https://docs.microsoft.com/en-us/cpp/build/reference/zc-rvaluecast-enforce-type-conversion-rules
        /Zc:sizedDealloc      # https://docs.microsoft.com/en-us/cpp/build/reference/zc-sizeddealloc-enable-global-sized-dealloc-functions
        /Zc:strictStrings     # https://docs.microsoft.com/en-us/cpp/build/reference/zc-strictstrings-disable-string-literal-type-conversion
        /Zc:threadSafeInit    # https://docs.microsoft.com/en-us/cpp/build/reference/zc-threadsafeinit-thread-safe-local-static-initialization
        /Zc:throwingNew       # https://docs.microsoft.com/en-us/cpp/build/reference/zc-throwingnew-assume-operator-new-throws
        /Zc:trigraphs-        # https://docs.microsoft.com/en-us/cpp/build/reference/zc-trigraphs-trigraphs-substitution
        /Zc:__cplusplus       # https://docs.microsoft.com/en-us/cpp/build/reference/zc-cplusplus
        /permissive-          # https://blogs.msdn.microsoft.com/vcblog/2016/11/16/permissive-switch/
                              # https://blogs.msdn.microsoft.com/vcblog/2017/09/11/two-phase-name-lookup-support-comes-to-msvc/
                              # https://docs.microsoft.com/en-us/cpp/build/reference/permissive-standards-conformance
        /volatile:iso         # https://docs.microsoft.com/en-us/cpp/build/reference/volatile-volatile-keyword-interpretation
        /GS
        /Zi
        /Gm-                  # Disable Minimal Rebuild
        # /sdl                 # Adds recommended Security Development Lifecycle (SDL) checks
        /EHsc
        /fp:precise
        /errorReport:prompt
        /GF                   # Eliminate Duplicate Strings
        /Gd                   # Specifies the __cdecl calling convention for all functions except C++ member functions and functions that are marked __stdcall, __fastcall, or __vectorcall.
        /diagnostics:caret    # https://docs.microsoft.com/en-us/cpp/build/reference/diagnostics-compiler-diagnostic-options
        /doc
        /wd4634               # XML document comment target: cannot be applied: reason
        /utf-8                # https://docs.microsoft.com/en-us/cpp/build/reference/utf-8-set-source-and-executable-character-sets-to-utf-8
        /WL                   # https://docs.microsoft.com/en-us/cpp/build/reference/wl-enable-one-line-diagnostics
    )

    if (CMAKE_CXX_FLAGS MATCHES "/W[0-3]")
        string(REGEX REPLACE "/W[0-3]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    else()
        add_compile_options(/W4)
    endif()

    add_definitions(
        -D_SCL_SECURE_NO_WARNINGS
        -D_CRT_SECURE_NO_WARNINGS
        -DNOMINMAX
        -D_WIN32_WINNT=0x0600   # Vista and above
        -D_WINDOWS
        -D_UNICODE
        -DUNICODE)

    # options for Debug
    set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS $<$<OR:$<CONFIG:Debug>,$<CONFIG:debug>>:_DEBUG DEBUG>)

    # options for Release & RelWithDebInfo
    # /O2 equivalent to /Og /Oi /Ot /Oy /Ob2 /Gs /GF /Gy
    # https://docs.microsoft.com/en-us/cpp/build/reference/o1-o2-minimize-size-maximize-speed
    # set_property(DIRECTORY APPEND PROPERTY COMPILE_OPTIONS $<$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>>:/O2>)
    add_compile_options($<$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>,$<CONFIG:release>>:/O2>)
    set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS $<$<OR:$<CONFIG:Release>,$<CONFIG:release>>:NDEBUG>)
else()
    message(FATAL_ERROR "Not supported C++ Compiler: " ${CMAKE_CXX_COMPILER_ID})
endif()

if (XENABLE_VERBOSE_DBG)
    add_definitions(-DXENABLE_VERBOSE_DBG)
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

if (BUILD_NTP)
    add_definitions(-DENABLE_NTP)
endif()

#add_definitions(-DEXTERNAL_PROPERTY)

if (XDISABLE_RATELIMIT)
    add_definitions(-DXDISABLE_RATELIMIT)
endif()

if (XENABLE_EXCEPTION_AS_ERROR)
    add_definitions(-DXTHROW_ERROR)
endif()

if (XENABLE_MOCK_ZEC_STAKE)
    add_definitions(-DXENABLE_MOCK_ZEC_STAKE)
endif()

if (ENABLE_SCALE)
    add_definitions(-DENABLE_SCALE)
endif()

if (XENABLE_CONFIG_CHECK)
    add_definitions(-DCONFIG_CHECK)
endif()

# for slash test
if (SLASH_TEST)
    add_definitions(-DSLASH_TEST)
endif()

# for workload test
if (WORKLOAD_TEST)
    add_definitions(-DWORKLOAD_TEST)
endif()

if (MAINNET_ACTIVATED)
    add_definitions(-DMAINNET_ACTIVATED)
endif()

if (XENABLE_BLOCK_VERIFY)
    add_definitions(-DENABLE_BLOCK_VERIFY)
endif()

if (MOCK_CA)
    add_definitions(-DMOCK_CA)
endif()

if (XSYNC_TEST)
    add_definitions(-DSYNC_TEST)
endif()

if (XENABLE_PSTACK)
    add_definitions(-DXENABLE_PSTACK)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -rdynamic")
endif()

if (XENABLE_MEMCHECK_DBG)
    add_definitions(-DXENABLE_MEMCHECK_DBG)
endif()

if (STATIC_CONSENSUS)
    if (NOT XENABLE_MOCK_ZEC_STAKE)
        message(FATAL_ERROR "should define mock_stake_zec before static_consensus")
    endif()
    add_definitions(-DSTATIC_CONSENSUS)
endif()

if (ELECT_WHEREAFTER)
    if (NOT STATIC_CONSENSUS)
        message(FATAL_ERROR "should define static_consensus before elect_whereafter")
    endif()
    add_definitions(-DELECT_WHEREAFTER)
endif()

if (CONSENSUS_SWAP)
    if (NOT ELECT_WHEREAFTER)
        message(FATAL_ERROR "should define elect_whereafter before consensus_swap")
    endif()
    add_definitions(-DCONSENSUS_SWAP)
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

if (LONG_CONFIRM_CHECK)
    add_definitions(-DLONG_CONFIRM_CHECK)
endif()

if (DB_KV_STATISTIC)
    add_definitions(-DDB_KV_STATISTIC)
endif()

if (DB_CACHE)
    add_definitions(-DDB_CACHE)
endif()

if (XENABLE_TESTS)
    add_definitions(-DXENABLE_TESTS)
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
    add_definitions(-DXCHAIN_FORKED_BY_DEFAULT)
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

if (${CMAKE_BUILD_TYPE} STREQUAL RelWithDebInfo)
    add_definitions(-DRELEASEDEBINFO)
endif()

if (${CMAKE_SYSTEM_NAME} STREQUAL Linux OR ${CMAKE_SYSTEM_NAME} STREQUAL Darwin)
#  find_package(PkgConfig REQUIRED)

endif()

message(STATUS "CMAKE_BUILD_TYPE:" ${CMAKE_BUILD_TYPE})
message(STATUS "CMAKE_SYSTEM_NAME:" ${CMAKE_SYSTEM_NAME})
message(STATUS "CMAKE_CXX_COMPILER_ID:" ${CMAKE_CXX_COMPILER_ID})
message(STATUS "XENABLE_CODE_COVERAGE:" ${XENABLE_CODE_COVERAGE})
message(STATUS "XENABLE_TESTS:" ${XENABLE_TESTS})
message(STATUS "XENABLE_VERBOSE_DBG:" ${XENABLE_VERBOSE_DBG})
message(STATUS "BUILD_METRICS:" ${BUILD_METRICS})
message(STATUS "ADDRESS_SANITIZER:" ${ADDRESS_SANITIZER})
message(STATUS "XCHAIN_FORKED_BY_DEFAULT:" ${XCHAIN_FORKED_BY_DEFAULT})
message(STATUS "DISABLE_CORE_SIGNAL_CAPTURE:" ${DISABLE_CORE_SIGNAL_CAPTURE})

find_package(Threads REQUIRED)

include_directories(src/xtopcom)  # xtopcom build warning will not print
include_directories(SYSTEM src/xtopcom/third_party)
include_directories(SYSTEM src/xtopcom/xdepends/include/trezor-crypto)
include_directories(SYSTEM src/xtopcom/xdepends/include)
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
    add_definitions(-DXDISABLE_RATELIMIT)
    add_definitions(-DXBUILD_DEV)
    add_definitions(-DXENABLE_MOCK_ZEC_STAKE)
    add_definitions(-DMAINNET_ACTIVATED)
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

if (XENABLE_TESTS)
    enable_testing()
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
