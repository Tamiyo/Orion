# Emits Substrait JSON from reduced ANF. Links YuzuAnf (the IR it consumes),
# which transitively provides the type system and LLVM Support (llvm::json).
add_library(YuzuSubstrait
  SubstraitEmitter.cc
)
target_link_libraries(YuzuSubstrait PUBLIC YuzuAnf)
