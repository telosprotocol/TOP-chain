aux_source_directory(./ main_src)
aux_source_directory(./xbasic_data/ xbasic_data_src)

add_executable(xbasic_data_test ${main_src} ${xbasic_data_src})

add_dependencies(xbasic_data_test xdata xconfig xutility xcommon xcrypto xbasic xxbase)

target_link_libraries(xbasic_data_test PRIVATE xdata xconfig xutility xcommon xcrypto xbasic xvledger jsoncpp xxbase pthread gtest)


