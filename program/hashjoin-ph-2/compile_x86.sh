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

cd src

# Compile seq-csr to assembly
clang -O3 -mllvm --x86-asm-syntax=intel -march=native -S npj2epb.c -o npj2epb.s
# Analyze the assembly with llvm-mca
llvm-mca -mcpu=native npj2epb.s > report.txt
parse_and_append_ipc "report.txt" "../../values.txt"
# Optionally compile the assembly file to an object file
clang -c npj2epb.s -o npj2epb.o
gcc -flto -g -std=c99 -Wall -O3 npj2epb.o main.c generator.c genzipf.c perf_counters.c cpu_mapping.c parallel_radix_join.c -lpthread -lm -std=c99  -o bin/x86/hj2-no

clang -O3 npj2epb.c -Xclang -load -Xclang ../../../freshAttempt/build/swPrefetchPass/SwPrefetchPass.so -c -S -emit-llvm 
clang -O3 npj2epb.ll -c 
clang -O3 npj2epb.o main.c generator.c genzipf.c perf_counters.c cpu_mapping.c parallel_radix_join.c -lpthread -lm -std=c99  -o bin/x86/hj2-auto

clang -O3 npj2epb.c -Xclang -load -Xclang ../../../freshAttempt/build/swPrefetchPass/SwPrefetchPass_new.so -c -S -emit-llvm 
clang -O3 npj2epb.ll -c 
clang -O3 npj2epb.o main.c generator.c genzipf.c perf_counters.c cpu_mapping.c parallel_radix_join.c -lpthread -lm -std=c99  -o bin/x86/hj2-auto-new

