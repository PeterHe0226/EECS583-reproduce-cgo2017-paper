clang -emit-llvm -S src/npj2epb.c -Xclang -disable-O0-optnone -o test.bc
opt -passes=mem2reg test.bc -S -o test.ll
opt -load-pass-plugin=./../../freshAttempt/build/swPrefetchPass/SwPrefetchPass_hj2.so -passes="func-name" test.ll -S -o testPost.ll