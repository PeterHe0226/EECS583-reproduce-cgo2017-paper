# This is the original implementation from the paper
add_llvm_pass_plugin(SwPrefetchPass swPrefetchPass.cpp)

# This is our implementation which computes a system specific C Const
add_llvm_pass_plugin(SwPrefetchPass_new swPrefetchPass.cpp)
target_compile_definitions(SwPrefetchPass_new PRIVATE -DCOMPUTE_C_CONST)