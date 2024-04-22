#!/usr/bin/env python3
import subprocess
import os.path
import time
import argparse
import pathlib
from datetime import datetime

BENCHMARK_OUTPUT_DIR = pathlib.Path("./benchmark_output")

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

def run_benchmark(commands, workdir):
    global args
    global timestamp
    if (args.save):
        output_dir = BENCHMARK_OUTPUT_DIR / workdir.strip("/").split("/")[-1] / timestamp
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

def run_graph500_benchmark():
    commands = [["bin/x86/g500-no"], ["bin/x86/g500-auto"], ["bin/x86/g500-auto-new"]]
    workdir = "./program/graph500"
    run_benchmark(commands, workdir)

def run_hj2_benchmark():
    commands = [["src/bin/x86/hj2-no"], ["src/bin/x86/hj2-auto"], ["src/bin/x86/hj2-auto-new"]]
    workdir = "./program/hashjoin-ph-2"
    run_benchmark(commands, workdir)

def run_hj8_benchmark():
    commands = [["src/bin/x86/hj2-no"], ["src/bin/x86/hj2-auto"], ["src/bin/x86/hj2-auto-new"]]
    workdir = "./program/hashjoin-ph-8"
    run_benchmark(commands, workdir)

def run_nas_cg_benchmark():
    commands = [["bin/x86/cg-no"], ["bin/x86/cg-auto"], ["bin/x86/cg-auto-new"]]
    workdir = "./program/nas-cg"
    run_benchmark(commands, workdir)

def run_randacc_benchmark():
    pass
    # TODO these commands take an argument. Need to align on what are sensible parameters
    # commands = [["bin/x86/randacc-no"], ["bin/x86/randacc-auto"], ["bin/x86/randacc-auto-new"]]
    # workdir = "./program/randacc"
    # run_benchmark(commands, workdir)

def run_all_benchmarks():
    run_graph500_benchmark()
    run_hj2_benchmark()
    run_hj8_benchmark()
    run_nas_cg_benchmark()
    run_randacc_benchmark()

if __name__ == "__main__":
    global args
    global timestamp
    args = parse_args()
    original_exists = os.path.isfile("./freshAttempt/build/swPrefetchPass/SwPrefetchPass.so")
    new_exists      = os.path.isfile("./freshAttempt/build/swPrefetchPass/SwPrefetchPass_new.so")

    if not original_exists or not new_exists:
        print("!!! Required shared libs not found... building them now !!!")
        time.sleep(1)
        do_cmd(['python3', 'build_pass.py', '-c'], './', False)

    execute = True
    while execute:
        timestamp = datetime.now().strftime("%m-%d-%y_%I:%M%p")
        print("a - run all benchmarks")
        print("b - build all tests")
        print("1 - run graph500  benchmark")
        print("2 - run hashjoin2 benchmark")
        print("3 - run hashjoin8 benchmark")
        print("4 - run nas-cg    benchmark")
        print("5 - run randacc   benchmark")
        print(f"s - toggle benchmark output save (currently {'ON' if args.save else 'OFF'})")
        print("e - exit")

        user_in = input(">> ")
        print("")

        match user_in:
            case 'a':
                run_all_benchmarks()
            case 'b':
                build_benchmarks()
            case '1':
                run_graph500_benchmark()
            case '2':
                run_hj2_benchmark()
            case '3':
                run_hj8_benchmark()
            case '4':
                run_nas_cg_benchmark()
            case '5':
                run_randacc_benchmark()
            case 's':
                args.save = not args.save
            case 'e':
                execute = False
            case _:
                print("invalid option!")
        
        print("")
