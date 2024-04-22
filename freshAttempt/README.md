Commands to run demo example (after pass is built):

clang -emit-llvm -S demoExample.c -Xclang -disable-O0-optnone -o demoExample.bc

opt -passes=mem2reg demoExample.bc -S -o demoExample.ll

opt -load-pass-plugin=./build/swPrefetchPass/SwPrefetchPass.so -passes="func-name" demoExample.ll -S -o demoPostPass.ll
