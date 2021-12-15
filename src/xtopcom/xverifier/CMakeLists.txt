add_compile_options(-static)

aux_source_directory(./ verifier_dir)
aux_source_directory(./src verifier_src)
add_library(xverifier STATIC ${verifier_dir} ${verifier_src})

#add_dependencies(xverifier xdata xcommon)

target_link_libraries(xverifier PRIVATE xxbase xdata xcommon xvledger )
