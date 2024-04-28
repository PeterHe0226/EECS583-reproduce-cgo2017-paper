[ ! -d "./bin" ] && mkdir ./bin
[ ! -d "./bin/x86" ] && mkdir ./bin/x86

clang -O3 cg.c -c
clang -O3 cg.o ../nas-common/c_print_results.c ../nas-common/c_timers.c ../nas-common/wtime.c -lm ../nas-common/c_randdp.c -o bin/x86/cg-no
clang -O3 cg.c -S -emit-llvm  -Xclang -load -Xclang ../../freshAttempt/build/swPrefetchPass/SwPrefetchPass.so
clang -O3 cg.ll -c
clang -O3 cg.o ../nas-common/c_print_results.c ../nas-common/c_timers.c ../nas-common/wtime.c -lm ../nas-common/c_randdp.c -o bin/x86/cg-auto
clang -O3 cg.c -S -emit-llvm  -Xclang -load -Xclang ../../freshAttempt/build/swPrefetchPass/SwPrefetchPass_cg.so
clang -O3 cg.ll -c
clang -O3 cg.o ../nas-common/c_print_results.c ../nas-common/c_timers.c ../nas-common/wtime.c -lm ../nas-common/c_randdp.c -o bin/x86/cg-auto-new