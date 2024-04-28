# Function to parse IPC from a report and append to a file
parse_and_append_ipc() {
    local report_file="$1"
    local output_file="$2"

    # Check if the report file exists
    if [ ! -f "$report_file" ]; then
        echo "Error: Report file $report_file does not exist."
        return 1
    fi

    # Parse the IPC value using grep and awk
    local ipc_value=$(grep "IPC:" "$report_file" | awk '{print $2}')

    # Check if IPC value was found
    if [ -z "$ipc_value" ]; then
        echo "IPC value not found in the report."
        return 1
    fi

    # save the IPC value to the output file
    echo "$ipc_value" >> "$output_file"

}

clang -O3 cg.c -c

# Compile seq-csr to assembly
clang -O3 -mllvm --x86-asm-syntax=intel -march=native -S cg.ll -o cg.s
# Analyze the assembly with llvm-mca
llvm-mca -mcpu=native cg.s > report.txt
parse_and_append_ipc "report.txt" "../../values.txt"
# Optionally compile the assembly file to an object file
clang -O3 -c cg.s -o cg.o
clang -O3 cg.o ../nas-common/c_print_results.c ../nas-common/c_timers.c ../nas-common/wtime.c -lm ../nas-common/c_randdp.c -o bin/x86/cg-auto-new

clang -O3 cg.ll -c
clang -O3 cg.o ../nas-common/c_print_results.c ../nas-common/c_timers.c ../nas-common/wtime.c -lm ../nas-common/c_randdp.c -o bin/x86/cg-no
clang -O3 cg.c -S -emit-llvm  -Xclang -load -Xclang ../../freshAttempt/build/swPrefetchPass/SwPrefetchPass.so
clang -O3 cg.ll -c
clang -O3 cg.o ../nas-common/c_print_results.c ../nas-common/c_timers.c ../nas-common/wtime.c -lm ../nas-common/c_randdp.c -o bin/x86/cg-auto
clang -O3 cg.c -S -emit-llvm  -Xclang -load -Xclang ../../freshAttempt/build/swPrefetchPass/SwPrefetchPass_new.so
