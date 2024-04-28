clang -O3 seq-csr/seq-csr.c -Xclang -load -Xclang ../../freshAttempt/build/swPrefetchPass/SwPrefetchPass.so -c -S -emit-llvm 
clang -O3 seq-csr.ll -c 
gcc -flto -g -std=c99 -Wall -O3 -I./generator   seq-csr.o graph500.c options.c rmat.c kronecker.c verify.c prng.c xalloc.c timer.c generator/splittable_mrg.c generator/graph_generator.c generator/make_graph.c generator/utils.c  -lm -lrt -o bin/x86/g500-auto

clang -O3 seq-csr/seq-csr.c -Xclang -load -Xclang ../../freshAttempt/build/swPrefetchPass/SwPrefetchPass_g500.so -c -S -emit-llvm 
clang -O3 seq-csr.ll -c 
gcc -flto -g -std=c99 -Wall -O3 -I./generator   seq-csr.o graph500.c options.c rmat.c kronecker.c verify.c prng.c xalloc.c timer.c generator/splittable_mrg.c generator/graph_generator.c generator/make_graph.c generator/utils.c  -lm -lrt -o bin/x86/g500-auto-new

clang -O3 seq-csr/seq-csr.c -c 
gcc -flto -g -std=c99 -Wall -O3 -I./generator   seq-csr.o graph500.c options.c rmat.c kronecker.c verify.c prng.c xalloc.c timer.c generator/splittable_mrg.c generator/graph_generator.c generator/make_graph.c generator/utils.c  -lm -lrt -o bin/x86/g500-no