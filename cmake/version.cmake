# get version, set version compile option

if(EXISTS "${PROJECT_SOURCE_DIR}/src/xtopcom/xversion/version")
    file(READ "${PROJECT_SOURCE_DIR}/src/xtopcom/xversion/version" PROGRAM_VERSION)
    string(STRIP "${PROGRAM_VERSION}" PROGRAM_VERSION)
    message("compile program version: ${PROGRAM_VERSION}")

    configure_file(
      ${PROJECT_SOURCE_DIR}/src/xtopcom/xversion/version.h.in
      generated/version.h
      @ONLY
      )
else()
    message(FATAL_ERROR "File ${PROJECT_SOURCE_DIR}/src/xtopcom/xversion/version not found")
endif()


