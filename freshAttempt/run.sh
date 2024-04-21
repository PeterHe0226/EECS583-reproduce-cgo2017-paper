#!/bin/bash
# e.g. sh run.sh hw2correct1

# ACTION NEEDED: If the path is different, please update it here.
PATH2LIB="./build/swPrefetchPass/SwPrefetchPass.so"        # Specify your build directory in the project

# ACTION NEEDED: Choose the correct pass when running.
PASS=func-name                

# Delete outputs from previous runs. Update this when you want to retain some files.
rm -f default.profraw *_prof *_fplicm *.bc *.profdata *_output *.ll

# Convert source code to bitcode (IR).
clang -emit-llvm -S ${1}.c -Xclang -disable-O0-optnone -o ${1}.ll

opt -load-pass-plugin="${PATH2LIB}" -passes="${PASS}" ${1}.ll -o ${1}.bc > /dev/null

# Generate binary excutable for optimized code
clang ${1}.bc -O0 -o ${1}-opt 
# Generate binary executable for unoptimized
clang ${1}.ll -O0 -o ${1}-no-opt

# # Produce output from binary to check correctness
./${1}-opt > opt_output
./${1}-no-opt > correct_output

echo -e "\n=== Program Correctness Validation ==="
if [ "$(diff correct_output opt_output)" != "" ]; then
    echo -e ">> Outputs do not match\n"
else
    echo -e ">> Outputs match\n"
    # Measure performance
    echo -e "1. Performance of unoptimized code"
    time ./${1}-no-opt > /dev/null
    echo -e "\n\n"
    echo -e "2. Performance of optimized code"
    time ./${1}-opt  > /dev/null
    echo -e "\n\n"
fi

# # Cleanup: Remove this if you want to retain the created files. And you do need to.
rm -f default.profraw *_prof *_fplicm *.bc *.profdata *_output *.ll ${1}-no-opt ${1}-opt 
