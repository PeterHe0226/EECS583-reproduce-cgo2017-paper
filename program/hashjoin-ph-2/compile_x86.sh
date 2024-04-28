cd src

[ ! -d "./bin" ] && mkdir ./bin
[ ! -d "./bin/x86" ] && mkdir ./bin/x86

clang -O3 npj2epb.c -Xclang -load -Xclang ../../../freshAttempt/build/swPrefetchPass/SwPrefetchPass.so -c -S -emit-llvm 
clang -O3 npj2epb.ll -c 
clang -O3 npj2epb.o main.c generator.c genzipf.c perf_counters.c cpu_mapping.c parallel_radix_join.c -lpthread -lm -std=c99  -o bin/x86/hj2-auto

clang -O3 npj2epb.c -Xclang -load -Xclang ../../../freshAttempt/build/swPrefetchPass/SwPrefetchPass_hj2.so -c -S -emit-llvm 
clang -O3 npj2epb.ll -c 
clang -O3 npj2epb.o main.c generator.c genzipf.c perf_counters.c cpu_mapping.c parallel_radix_join.c -lpthread -lm -std=c99  -o bin/x86/hj2-auto-new

clang -O3 npj2epb.c -c 
clang -O3 npj2epb.o main.c generator.c genzipf.c perf_counters.c cpu_mapping.c parallel_radix_join.c -lpthread -lm -std=c99  -o bin/x86/hj2-no