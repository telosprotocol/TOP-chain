#get svn and build infomation that will be written to Src/Version.h
#call after command project for ${PROJECT_SOURCE_DIR}
macro(get_git_info)
    find_package(Git)

    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE TOP_GIT_BRANCH
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    # execute_process(COMMAND ${GIT_EXECUTABLE} log -1 --format=%H
    #     WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    #     OUTPUT_VARIABLE TOP_GIT_HASH
    #     OUTPUT_STRIP_TRAILING_WHITESPACE
    # )

    # execute_process(COMMAND ${GIT_EXECUTABLE} log -1 --format=%cd
    #     WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    #     OUTPUT_VARIABLE TOP_GIT_DATE
    #     OUTPUT_STRIP_TRAILING_WHITESPACE
    # )

    #execute_process(COMMAND ${GIT_EXECUTABLE} log -1 --pretty=oneline --abbrev-commit
    execute_process(COMMAND ${GIT_EXECUTABLE} log -1 --pretty=format:"%h"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE TOP_GIT_LOG_LATEST
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    execute_process(
        COMMAND sh -c "id | awk '{printf $1}'"
        OUTPUT_VARIABLE TOP_BUILD_USER
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    execute_process(
        COMMAND sh -c "hostname | awk '{printf $1}'"
        OUTPUT_VARIABLE TOP_BUILD_HOST
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    execute_process(
        COMMAND sh -c "pwd | awk '{printf $1}'"
        OUTPUT_VARIABLE TOP_BUILD_PATH
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    execute_process(
        COMMAND sh -c "${GIT_EXECUTABLE} submodule foreach git log -1 --format='%h' | sed 'N;s/\\n/:/g' | grep -aoE 'xtopcom.*' | sed \"s/\'//g\" | sed 's/xtopcom\\\///g' | tr '\n' '|' "
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE TOP_GIT_SUBMODULE
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    
    # string (REGEX REPLACE "[\n\t\r]" "" TOP_GIT_BRANCH "${TOP_GIT_BRANCH}")
    # string (REGEX REPLACE "[\n\t\r]" "" TOP_GIT_HASH "${TOP_GIT_HASH}")
    # string (REGEX REPLACE "[\n\t\r]" "" TOP_GIT_DATE "${TOP_GIT_DATE}")

    message("git branch: ${TOP_GIT_BRANCH}")
    # message("git hash: ${TOP_GIT_HASH}")
    # message("git date: ${TOP_GIT_DATE}")
    message("git log -1: ")
    message("${TOP_GIT_LOG_LATEST}")
    message("git submodule info:")
    message("${TOP_GIT_SUBMODULE}")

    set(BUILD_OPTIONS_LIST "unknown")
    if (EXISTS ${PROJECT_BINARY_DIR}/build_options.inc)
        file(READ ${PROJECT_BINARY_DIR}/build_options.inc BUILD_OPTIONS_LIST)
        message("build options: ${BUILD_OPTIONS_LIST}")
    else()
        message("not find build_options file (should be generate by build.sh)")
    endif()

    set(VersionInc "${PROJECT_SOURCE_DIR}/src/xtopcom/xversion/src/version.inc")
    file(WRITE  ${VersionInc} "const std::string TOP_VERSION_PREFIX  = \"TOP_VERSION\";\n")  # will overwrite
    file(APPEND ${VersionInc} "const std::string TOP_GIT_BRANCH      = \"${TOP_GIT_BRANCH}\";\n")
    file(APPEND ${VersionInc} "const std::string TOP_GIT_HASH        = \"${TOP_GIT_HASH}\";\n")
    file(APPEND ${VersionInc} "const std::string TOP_GIT_DATE        = \"${TOP_GIT_DATE}\";\n")
    file(APPEND ${VersionInc} "const std::string TOP_GIT_LOG_LATEST  = ${TOP_GIT_LOG_LATEST};\n")  # multiline
    file(APPEND ${VersionInc} "const std::string TOP_GIT_SUBMODULE   = \"${TOP_GIT_SUBMODULE}\";\n")
    file(APPEND ${VersionInc} "const std::string TOP_BUILD_DATE      = __DATE__;\n")
    file(APPEND ${VersionInc} "const std::string TOP_BUILD_TIME      = __TIME__;\n")
    file(APPEND ${VersionInc} "const std::string TOP_BUILD_USER      = \"${TOP_BUILD_USER}\";\n")
    file(APPEND ${VersionInc} "const std::string TOP_BUILD_HOST      = \"${TOP_BUILD_HOST}\";\n")
    file(APPEND ${VersionInc} "const std::string TOP_BUILD_PATH      = \"${TOP_BUILD_PATH}\";\n")
    file(APPEND ${VersionInc} "const std::string TOP_BUILD_OPTIONS   = \"${BUILD_OPTIONS_LIST}\";\n")

endmacro()
