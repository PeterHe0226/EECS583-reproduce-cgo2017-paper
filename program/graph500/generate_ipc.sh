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
    echo "graph500: $ipc_value" >> "$output_file"

}

# Compile seq-csr to assembly
clang -O3 -mllvm --x86-asm-syntax=intel -march=native -S seq-csr/seq-csr.c -o seq-csr.s
# Analyze the assembly with llvm-mca
llvm-mca -mcpu=native seq-csr.s > report.txt
parse_and_append_ipc "report.txt" "../../values.txt"