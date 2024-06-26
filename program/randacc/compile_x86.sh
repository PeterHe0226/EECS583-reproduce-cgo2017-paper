[ ! -d "./bin" ] && mkdir ./bin
[ ! -d "./bin/x86" ] && mkdir ./bin/x86

clang -O3 randacc.c -Xclang -load -Xclang ../../freshAttempt/build/swPrefetchPass/SwPrefetchPass.so -c -S -emit-llvm
clang -O3 randacc.ll -o bin/x86/randacc-auto

clang -O3 randacc.c -Xclang -load -Xclang ../../freshAttempt/build/swPrefetchPass/SwPrefetchPass_rand.so -c -S -emit-llvm
clang -O3 randacc.ll -o bin/x86/randacc-auto-new

clang -O3 randacc.c -o bin/x86/randacc-no