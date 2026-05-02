llvm_map_components_to_libnames(yuzu_util_llvm_libs Support)

add_library(YuzuUtil INTERFACE)
target_link_libraries(YuzuUtil INTERFACE ${yuzu_util_llvm_libs})
