#!/usr/bin/env python3
import subprocess
import os.path
import time
import argparse
import pathlib
from datetime import datetime
import re
import shutil
import csv
import matplotlib.pyplot as plt

BENCHMARK_OUTPUT_DIR = pathlib.Path("./benchmark_output")
BENCHMARK_TEMP_OUTPUT_DIR = pathlib.Path("./temp_output")

TEST_REPITITIONS = 10

def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("-s", "--save", help="Save benchmark output in benchmark_output/", action="store_true")
    return parser.parse_args()

def do_cmd(command, dir, use_shell=True):
    p = subprocess.Popen(command, cwd=dir, shell=use_shell)
    p.wait()

def build_benchmarks():
    do_cmd(["./compile_x86.sh"], "./program/graph500")
    do_cmd(["./compile_x86.sh"], "./program/hashjoin-ph-2")
    do_cmd(["./compile_x86.sh"], "./program/hashjoin-ph-8")
    do_cmd(["./compile_x86.sh"], "./program/nas-cg")
    do_cmd(["./compile_x86.sh"], "./program/randacc")

def generate_output_dir(workdir):
    global timestamp
    global args

    out_dir = BENCHMARK_TEMP_OUTPUT_DIR
    if (args.save):
        out_dir = BENCHMARK_OUTPUT_DIR

    output_dir = out_dir / workdir.strip("/").split("/")[-1] / timestamp

    return output_dir

def run_benchmark(commands, workdir):
    global args
    global timestamp

    try:
        output_dir = generate_output_dir(workdir)
        output_dir.mkdir(parents=True) 
        output_dir = output_dir.resolve()
        
        output_commands = []
        for command in commands:
            executable_name = command[0].split('/')[-1]
            command.extend(["|", f"tee {output_dir / executable_name}.txt"])
            output_commands.append(' '.join(command))
        commands = output_commands
    except:
        pass

    for command in commands:
        do_cmd(command, workdir)

def get_files_to_parse(commands, workdir):
    global args
    
    files = []
    
    output_dir = generate_output_dir(workdir)
    for command in commands:
        file = output_dir / (command[0].split('/')[-1] + ".txt")
        files.append(file)

    return files

def create_pretty_print_text(benchmark, times):
    s  = '\n'
    if (len(times) > 1):
        s += '*********************************************************\n'
        s += 'Benchmark:    ' + benchmark + '\n'
        s +=  'Normal:       ' + str(times[0]) + '\n'
        s +=  'Prefetch:     ' + str(times[1]) + '\n'
        s +=  'New Prefetch: ' + str(times[2]) + '\n'
        s += '*********************************************************\n'
        s += '\n'

    return s

def try_parse_results(commands, workdir, regex):
    global args

    files = get_files_to_parse(commands, workdir)
    times = []
    for file in files:
        with open(file, 'r') as f:
            data = f.read()
            match = re.search(regex, data)
            if match:
                times.append(float(match.group(1)))

    if not args.save:
        # delete the temp out director
        dir = generate_output_dir(workdir)
        shutil.rmtree(dir)

    return times

def repeat_benchmark(commands, workdir, regex):
    total_times = [0] * len(commands)
    test_name = workdir.split('/')[-1]
    for i in range(TEST_REPITITIONS):
        print("")
        print(test_name + " Benchmark Run #" + str(i + 1))
        print("")
        run_benchmark(commands, workdir)
        temp = try_parse_results(commands, workdir, regex)
        for i in range(len(total_times)):
            total_times[i] += temp[i]

    for i in range(len(total_times)):
        total_times[i] /= TEST_REPITITIONS

    return create_pretty_print_text(test_name, total_times), total_times

def run_graph500_benchmark(run_only_standard_prefetch = False):
    if run_only_standard_prefetch:
        commands = [["bin/x86/g500-auto"]]
    else:
        commands = [["bin/x86/g500-no"], ["bin/x86/g500-auto"], ["bin/x86/g500-auto-new"]]

    workdir = "./program/graph500"

    return repeat_benchmark(commands, workdir, r"median_time: (\d+\.\d+e[+-]\d+)")

def run_hj2_benchmark(run_only_standard_prefetch = False):
    if run_only_standard_prefetch:
        commands = [["src/bin/x86/hj2-auto"]]
    else:
        commands = [["src/bin/x86/hj2-no"], ["src/bin/x86/hj2-auto"], ["src/bin/x86/hj2-auto-new"]]
    
    workdir = "./program/hashjoin-ph-2"
    
    return repeat_benchmark(commands, workdir, r"TOTAL-TIME-USECS, TOTAL-TUPLES, CYCLES-PER-TUPLE:\s+(\d+\.\d+)\s+")

def run_hj8_benchmark(run_only_standard_prefetch = False):
    if run_only_standard_prefetch:
        commands = [["src/bin/x86/hj2-auto"]]
    else:
        commands = [["src/bin/x86/hj2-no"], ["src/bin/x86/hj2-auto"], ["src/bin/x86/hj2-auto-new"]]

    workdir = "./program/hashjoin-ph-8"
    
    return repeat_benchmark(commands, workdir, r"TOTAL-TIME-USECS, TOTAL-TUPLES, CYCLES-PER-TUPLE:\s+(\d+\.\d+)\s+")

def run_nas_cg_benchmark(run_only_standard_prefetch = False):
    if run_only_standard_prefetch:
        commands = [["bin/x86/cg-auto"]]
    else:
        commands = [["bin/x86/cg-no"], ["bin/x86/cg-auto"], ["bin/x86/cg-auto-new"]]
    
    workdir = "./program/nas-cg"

    return repeat_benchmark(commands, workdir, r"Time in seconds\s*=\s*(\d+\.\d+)")

def run_randacc_benchmark(run_only_standard_prefetch = False):
    if run_only_standard_prefetch:
        commands = [["bin/x86/randacc-auto", "100000000"]]
    else:
        commands = [["bin/x86/randacc-no", "100000000"], ["bin/x86/randacc-auto", "100000000"], ["bin/x86/randacc-auto-new", "100000000"]]

    workdir = "./program/randacc"
    
    return repeat_benchmark(commands, workdir, r"Time in milliseconds:\s*(\d+\.\d+)")

def modify_cpp_constant(new_value):

    pattern = re.compile(r'^#define C_CONSTANT \(\d+\)$')

    file_path = "./freshAttempt/swPrefetchPass/swPrefetchPass.cpp" 

    lines = []

    with open(file_path, 'r') as file:
        lines = file.readlines()

    
    with open(file_path, 'w') as file:
        for line in lines:
            if pattern.match(line.strip()):
                # Replace the existing value with the new value
                line = f'#define C_CONSTANT ({new_value})\n'
            file.write(line)

def plot_optimal_c_value_data(file_path):
    with open(file_path, 'r') as file:
        csv_reader = csv.reader(file)
        headers = next(csv_reader)  # Read the header row
        x_labels = headers[1:]  # Column headers except the first 'test' column

        plt.figure(constrained_layout=True)
        
        for row in csv_reader:
            test = row[0]
            y_values = list(map(float, row[1:]))
            # Normalize by dividing by the first data point in the row
            base_value = y_values[0]
            normalized_values = [y / base_value for y in y_values]
            
            plt.plot(x_labels, normalized_values, marker='o', label=test)
        
        plt.title("Optimal C Constant Value for benchmarks")
        plt.xlabel("C Constant Value")
        plt.ylabel("Time Relative to Performance for C=32")
        plt.grid(True)
        plt.legend()  # Add a legend to identify each line
        plt.savefig('optimal_c_const_value.png')

def find_optimal_c_value():
    current = 0

    g500 = []
    hj2  = []
    hj8  = []
    cg   = []
    rand = []

    for i in range(8): 
        modify_cpp_constant(current)
        rebuild_pass()
        build_benchmarks()
        ignore, times = run_all_benchmarks(True)

        g500.append(times[0][0])
        hj2.append(times[1][0])
        hj8.append(times[2][0])
        cg.append(times[3][0])
        rand.append(times[4][0])

        current = current + 32

    out_file = "./optimal_c_value_out.csv"

    with open(out_file, mode='w', newline='') as file:
        writer = csv.writer(file)

        writer.writerow(['test', '32', '64', '96','128','160','192','224', '256'])

        temp = g500
        writer.writerow(['g500', temp[0], temp[1], temp[2], temp[3], temp[4], temp[5], temp[6], temp[7]])
        temp = hj2
        writer.writerow(['hj2', temp[0], temp[1], temp[2], temp[3], temp[4], temp[5], temp[6], temp[7]])
        temp = hj8
        writer.writerow(['hj8', temp[0], temp[1], temp[2], temp[3], temp[4], temp[5], temp[6], temp[7]])
        temp = cg
        writer.writerow(['cg', temp[0], temp[1], temp[2], temp[3], temp[4], temp[5], temp[6], temp[7]])
        temp = rand
        writer.writerow(['rand', temp[0], temp[1], temp[2], temp[3], temp[4], temp[5], temp[6], temp[7]])

    # restore original value in cpp file, pass, and benchmarks
    modify_cpp_constant(64)
    rebuild_pass()
    build_benchmarks()

    plot_optimal_c_value_data(out_file)

def rebuild_pass():
    do_cmd(['python3', 'build_pass.py', '-c'], './', False)

def run_all_benchmarks(run_only_standard_prefetch = False):
    results = ''
    times = []
    s, t = run_graph500_benchmark(run_only_standard_prefetch)
    results += s
    times.append(t)
    s, t = run_hj2_benchmark(run_only_standard_prefetch)
    results += s
    times.append(t)
    s, t = run_hj8_benchmark(run_only_standard_prefetch)
    results += s
    times.append(t)
    s, t = run_nas_cg_benchmark(run_only_standard_prefetch)
    results += s
    times.append(t)
    s, t = run_randacc_benchmark(run_only_standard_prefetch)
    results += s
    times.append(t)
    return results, times

def generate_all_ipc():
    if os.path.exists("values.txt"):
        os.remove("values.txt")
    do_cmd(["./generate_ipc.sh"], "./program/graph500")
    do_cmd(["./generate_ipc.sh"], "./program/hashjoin-ph-2")
    do_cmd(["./generate_ipc.sh"], "./program/hashjoin-ph-8")
    do_cmd(["./generate_ipc.sh"], "./program/nas-cg")
    do_cmd(["./generate_ipc.sh"], "./program/randacc")

if __name__ == "__main__":
    global args
    global timestamp
    args = parse_args()
    libs_exists = os.path.isfile("./freshAttempt/build/swPrefetchPass/SwPrefetchPass.so")

    if not libs_exists:
        print("!!! Required shared libs not found... building them now !!!")
        time.sleep(1)
        generate_all_ipc()
        rebuild_pass()

    execute = True
    while execute:
        timestamp = datetime.now().strftime("%m-%d-%y_%I:%M%p")
        print("a - run all benchmarks")
        print("p - rebuild the pass")
        print("t - build all benchmarks")
        print("b - rebuild the pass and all benchmarks")
        print("f - find optimal c value")
        print("i - generate ipc for all benchmarks")
        print("1 - run graph500  benchmark")
        print("2 - run hashjoin2 benchmark")
        print("3 - run hashjoin8 benchmark")
        print("4 - run nas-cg    benchmark")
        print("5 - run randacc   benchmark")
        print(f"s - toggle benchmark output save (currently {'ON' if args.save else 'OFF'})")
        print("e - exit")

        user_in = input(">> ")
        print("")

        results = ''

        match user_in:
            case 'a':
                results, ignore = run_all_benchmarks()
            case 'p':
                rebuild_pass()
            case 't':
                build_benchmarks()
            case 'b':
                generate_all_ipc()
                rebuild_pass()
                build_benchmarks()
            case 'i':
                generate_all_ipc()
            case 'f':
                find_optimal_c_value()
            case '1':
                results, ignore = run_graph500_benchmark()
            case '2':
                results, ignore = run_hj2_benchmark()
            case '3':
                results, ignore = run_hj8_benchmark()
            case '4':
                results, ignore = run_nas_cg_benchmark()
            case '5':
                results, ignore = run_randacc_benchmark()
            case 's':
                args.save = not args.save
            case 'e':
                execute = False
            case _:
                print("invalid option!")
        
        print(results)
