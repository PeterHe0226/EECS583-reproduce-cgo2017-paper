# Overview
`demoExample.c` = sample program that has opportunity for prefetching  
  
`demoExample.ll` = IR before it goes through the prefetching pass  
  
`demoPostPass.ll` = IR after it goes through our prefetching pass with dynamic computation of the C constant  

# Requirements
Requires that the pass has been built. See instructions under freshAttempt/

# Run Demo
Commands to run demo example (after pass is built):

clang -emit-llvm -S demoExample.c -Xclang -disable-O0-optnone -o demoExample.bc

opt -passes=mem2reg demoExample.bc -S -o demoExample.ll

opt -load-pass-plugin=./../build/swPrefetchPass/SwPrefetchPass_g500.so -passes="func-name" demoExample.ll -S -o demoPostPass.ll  
