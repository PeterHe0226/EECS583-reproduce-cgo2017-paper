# This is the original implementation from the paper
add_llvm_pass_plugin(SwPrefetchPass swPrefetchPass.cpp)

# This is our implementation which computes a system specific C Const
add_llvm_pass_plugin(SwPrefetchPass_g500 swPrefetchPass.cpp)
target_compile_definitions(SwPrefetchPass_g500 PRIVATE -DCOMPUTE_C_CONST -DIPC_INDEX=0)

add_llvm_pass_plugin(SwPrefetchPass_hj2 swPrefetchPass.cpp)
target_compile_definitions(SwPrefetchPass_hj2 PRIVATE -DCOMPUTE_C_CONST -DIPC_INDEX=1)

add_llvm_pass_plugin(SwPrefetchPass_hj8 swPrefetchPass.cpp)
target_compile_definitions(SwPrefetchPass_hj8 PRIVATE -DCOMPUTE_C_CONST -DIPC_INDEX=2)

add_llvm_pass_plugin(SwPrefetchPass_cg swPrefetchPass.cpp)
target_compile_definitions(SwPrefetchPass_cg PRIVATE -DCOMPUTE_C_CONST -DIPC_INDEX=3)

add_llvm_pass_plugin(SwPrefetchPass_rand swPrefetchPass.cpp)
target_compile_definitions(SwPrefetchPass_rand PRIVATE -DCOMPUTE_C_CONST -DIPC_INDEX=4)