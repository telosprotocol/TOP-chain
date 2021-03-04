add_compile_options(-Wno-unused-parameter)

aux_source_directory(./src src)
add_executable(topio ${src})

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

#add_dependencies(topio xpbase)
target_link_libraries(topio PRIVATE xpbase jsoncpp pthread dl)
