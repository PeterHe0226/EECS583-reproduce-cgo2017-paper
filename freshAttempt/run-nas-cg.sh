#!/bin/bash
# e.g. sh run.sh hw2correct1

# ACTION NEEDED: If the path is different, please update it here.
DIR=$(pwd)
PATH2LIB="${DIR}/build/swPrefetchPass/SwPrefetchPass.so"        # Specify your build directory in the project

# ACTION NEEDED: Choose the correct pass when running.
PASS=func-name                

rm -f default.profraw *_prof *_fplicm *.bc *.profdata *_output *.ll

# ADAPTED FROM program/nas-cg/compile_x86.sh FOR BUILD STEPS
cd ../program/nas-cg 
# NO OPT
clang -O3 cg.c -c -o ${DIR}/cg.o
clang -O3 ${DIR}/cg.o ../nas-common/c_print_results.c ../nas-common/c_timers.c ../nas-common/wtime.c -lm ../nas-common/c_randdp.c -o ${DIR}/cg-no-opt

# OPT
clang -O3  -emit-llvm -S cg.c -Xclang -disable-O0-optnone -o ${DIR}/cg.ll
opt -loop-unroll-disable -load-pass-plugin="${PATH2LIB}" -passes="${PASS}" ${DIR}/cg.ll -o ${DIR}/cg.bc > /dev/null
clang -O3 ${DIR}/cg.bc -c
clang -O3 ${DIR}/cg.o ../nas-common/c_print_results.c ../nas-common/c_timers.c ../nas-common/wtime.c -lm ../nas-common/c_randdp.c -o ${DIR}/cg-opt

# # # Produce output from binary to check correctness
# ./${1}-opt > opt_output
# ./${1}-no-opt > correct_output

# echo -e "\n=== Program Correctness Validation ==="
# if [ "$(diff correct_output opt_output)" != "" ]; then
#     echo -e ">> Outputs do not match\n"
# else
#     echo -e ">> Outputs match\n"
#     # Measure performance
#     echo -e "1. Performance of unoptimized code"
#     time ./${1}-no-opt > /dev/null
#     echo -e "\n\n"
#     echo -e "2. Performance of optimized code"
#     time ./${1}-opt  > /dev/null
#     echo -e "\n\n"
# fi

# # Cleanup: Remove this if you want to retain the created files. And you do need to.


# clang -O3 CG/cgswpf.c -c -DFETCHDIST=64 -DSTRIDE
# clang -O3 cgswpf.o ../nas-common/c_print_results.c ../nas-common/c_timers.c ../nas-common/wtime.c -lm ../nas-common/c_randdp.c -o bin/x86/cg-man
# for i in 2 4 8 16 32 64 128 256 2048
# do
#   clang -O3 CG/cgswpf.c -c -DFETCHDIST=$i -DSTRIDE
#   clang -O3 cgswpf.o ../nas-common/c_print_results.c ../nas-common/c_timers.c ../nas-common/wtime.c -lm ../nas-common/c_randdp.c -o bin/x86-offsets/cg-offset-$i
# done
#   clang -O3 CG/cgswpf.c  -c -DFETCHDIST=64
#   clang -O3 cgswpf.o ../nas-common/c_print_results.c ../nas-common/c_timers.c ../nas-common/wtime.c -lm ../nas-common/c_randdp.c -o bin/x86-offsets/cg-offset-64-nostride

