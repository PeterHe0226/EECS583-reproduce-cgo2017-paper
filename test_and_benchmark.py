#!/usr/bin/env python3
import subprocess
import os.path
import time
import argparse
import pathlib
from datetime import datetime
import re
import shutil

BENCHMARK_OUTPUT_DIR = pathlib.Path("./benchmark_output")
BENCHMARK_TEMP_OUTPUT_DIR = pathlib.Path("./temp_output")

TEST_REPITITIONS = 3

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

    output_dir = generate_output_dir(workdir)
    output_dir.mkdir(parents=True) 
    output_dir = output_dir.resolve()
    
    output_commands = []
    for command in commands:
        executable_name = command[0].split('/')[-1]
        command.extend(["|", f"tee {output_dir / executable_name}.txt"])
        output_commands.append(' '.join(command))
    commands = output_commands

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
    total_times = [0, 0, 0]
    test_name = workdir.split('/')[-1]
    for i in range(TEST_REPITITIONS):
        print("")
        print(test_name + " Benchmark Run #" + str(i + 1))
        print("")
        run_benchmark(commands, workdir)
        temp = try_parse_results(commands, workdir, regex)
        total_times[0] += temp[0]
        total_times[1] += temp[1]
        total_times[2] += temp[2]

    total_times[0] /= TEST_REPITITIONS
    total_times[1] /= TEST_REPITITIONS
    total_times[2] /= TEST_REPITITIONS

    return create_pretty_print_text(test_name, total_times)

def run_graph500_benchmark():
    commands = [["bin/x86/g500-no"], ["bin/x86/g500-auto"], ["bin/x86/g500-auto-new"]]
    workdir = "./program/graph500"

    return repeat_benchmark(commands, workdir, r"median_time: (\d+\.\d+e[+-]\d+)")

def run_hj2_benchmark():
    commands = [["src/bin/x86/hj2-no"], ["src/bin/x86/hj2-auto"], ["src/bin/x86/hj2-auto-new"]]
    workdir = "./program/hashjoin-ph-2"
    
    return repeat_benchmark(commands, workdir, r"TOTAL-TIME-USECS, TOTAL-TUPLES, CYCLES-PER-TUPLE:\s+(\d+\.\d+)\s+")

def run_hj8_benchmark():
    commands = [["src/bin/x86/hj2-no"], ["src/bin/x86/hj2-auto"], ["src/bin/x86/hj2-auto-new"]]
    workdir = "./program/hashjoin-ph-8"
    
    return repeat_benchmark(commands, workdir, r"TOTAL-TIME-USECS, TOTAL-TUPLES, CYCLES-PER-TUPLE:\s+(\d+\.\d+)\s+")

def run_nas_cg_benchmark():
    commands = [["bin/x86/cg-no"], ["bin/x86/cg-auto"], ["bin/x86/cg-auto-new"]]
    workdir = "./program/nas-cg"

    return repeat_benchmark(commands, workdir, r"Time in seconds\s*=\s*(\d+\.\d+)")

def run_randacc_benchmark():
    return ''
    # TODO these commands take an argument. Need to align on what are sensible parameters
    # commands = [["bin/x86/randacc-no"], ["bin/x86/randacc-auto"], ["bin/x86/randacc-auto-new"]]
    # workdir = "./program/randacc"
    # run_benchmark(commands, workdir)

def rebuild_pass():
    do_cmd(['python3', 'build_pass.py', '-c'], './', False)

def run_all_benchmarks():
    results = ''
    results += run_graph500_benchmark()
    results += run_hj2_benchmark()
    results += run_hj8_benchmark()
    results += run_nas_cg_benchmark()
    results += run_randacc_benchmark()
    return results

if __name__ == "__main__":
    global args
    global timestamp
    args = parse_args()
    original_exists = os.path.isfile("./freshAttempt/build/swPrefetchPass/SwPrefetchPass.so")
    new_exists      = os.path.isfile("./freshAttempt/build/swPrefetchPass/SwPrefetchPass_new.so")

    if not original_exists or not new_exists:
        print("!!! Required shared libs not found... building them now !!!")
        time.sleep(1)
        rebuild_pass()

    execute = True
    while execute:
        timestamp = datetime.now().strftime("%m-%d-%y_%I:%M%p")
        print("a - run all benchmarks")
        print("p - rebuild the pass")
        print("t - build all benchmarks")
        print("b - rebuild the pass and all benchmarks")
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
                results = run_all_benchmarks()
            case 'p':
                rebuild_pass()
            case 't':
                build_benchmarks()
            case 'b':
                rebuild_pass()
                build_benchmarks()
            case '1':
                results = run_graph500_benchmark()
            case '2':
                results = run_hj2_benchmark()
            case '3':
                results = run_hj8_benchmark()
            case '4':
                results = run_nas_cg_benchmark()
            case '5':
                results = run_randacc_benchmark()
            case 's':
                args.save = not args.save
            case 'e':
                execute = False
            case _:
                print("invalid option!")
        
        print(results)
