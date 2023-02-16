aux_source_directory(./src xgenerate_account_src)

add_executable(xgenerate_account ${xgenerate_account_src})

#add_dependencies(xgenerate_account xcrypto xcommon xxbase)
target_link_libraries(xgenerate_account xcrypto xcommon xxbase)
