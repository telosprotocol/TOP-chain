#find_package(Boost REQUIRED)

add_compile_options(-Wpedantic)

aux_source_directory(./src src_dir)
aux_source_directory(./src/trie trie_dir)
aux_source_directory(./src/rlp rlp_dir)
add_library(xevm_common STATIC ${src_dir} ${trie_dir} ${rlp_dir})

add_dependencies(xevm_common xcommon xbasic)

target_link_libraries(xevm_common PRIVATE xcommon xbasic ethash keccak)
