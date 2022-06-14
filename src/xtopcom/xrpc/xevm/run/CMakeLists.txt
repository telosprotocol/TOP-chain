# Go cmake configuration
cmake_minimum_required(VERSION 3.8)

set(TARGET jsonrpc_go)

set(SRCS main.go)
set(LIB libjsonrpc.a)

add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${LIB}
  DEPENDS ${SRCS}
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  COMMAND env GOPATH=${GOPATH} go build -buildmode=c-archive
  -o "${CMAKE_CURRENT_BINARY_DIR}/${LIB}"
  ${CMAKE_GO_FLAGS} ./...
  COMMENT "Building Go library")

add_custom_target(${TARGET} DEPENDS ${LIB} ${HEADER})

add_library(jsonrpc STATIC IMPORTED GLOBAL)
add_dependencies(jsonrpc ${TARGET})
set_target_properties(jsonrpc
  PROPERTIES
  IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/${LIB}
  INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_BINARY_DIR})