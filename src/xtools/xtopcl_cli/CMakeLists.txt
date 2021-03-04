cmake_minimum_required(VERSION 3.8)

aux_source_directory(./src xtopcl_cli_src)

#compile definitions
add_definitions(
    -DUSE_STANDALONE_ASIO
    -DASIO_STANDALONE
)

# SET TARGET
add_executable(xtopcl_cli ${xtopcl_cli_src})
#add_dependencies(xtopcl_cli xxbase xdata xcrypto xtopcl)
target_link_libraries(xtopcl_cli PRIVATE xtopcl)
