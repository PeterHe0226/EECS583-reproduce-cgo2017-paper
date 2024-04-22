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

def do_cmd(command, dir):
    p = subprocess.Popen(command, cwd=dir, shell=True)
    p.wait()

def build_benchmarks():
    do_cmd(["./compile_x86.sh"], "./program/graph500")
    do_cmd(["./compile_x86.sh"], "./program/hashjoin-ph-2")
    do_cmd(["./compile_x86.sh"], "./program/hashjoin-ph-8")
    do_cmd(["./compile_x86.sh"], "./program/nas-cg")
    do_cmd(["./compile_x86.sh"], "./program/randacc")

def run_graph500_benchmark():
    print("TODO - graph500")

def run_hj2_benchmark():
    print("TODO - hj2")

def run_hj8_benchmark():
    print("TODO - hj8")

def run_nas_cg_benchmark(save):
    commands = [["bin/x86/cg-no"], ["bin/x86/cg-auto"], ["bin/x86/cg-auto-new"]]
    workdir = "./program/nas-cg"
    if (save):
        output_dir = BENCHMARK_OUTPUT_DIR / "nas-cg"
        if not output_dir.exists():
            output_dir.mkdir() 
        
        output_dir = output_dir.resolve()
        
        timestamp = datetime.now().strftime("%m-%d-%y_%I:%M%p")
        for command in commands:
            executable_name = command[0].split('/')[-1]
            command.extend(["|", f"tee {output_dir / (executable_name + timestamp)}.txt"])
            command = ' '.join(command)
            
    for command in commands:
        do_cmd(command, workdir)

def run_randacc_benchmark():
    print("TODO - randacc")

def run_all_benchmarks():
    run_graph500_benchmark()
    run_hj2_benchmark()
    run_hj8_benchmark()
    run_nas_cg_benchmark()
    run_randacc_benchmark()

if __name__ == "__main__":
    args = parse_args()
    original_exists = os.path.isfile("./freshAttempt/build/swPrefetchPass/SwPrefetchPass.so")
    new_exists      = os.path.isfile("./freshAttempt/build/swPrefetchPass/SwPrefetchPass_new.so")

    if not original_exists or not new_exists:
        print("!!! Required shared libs not found... building them now !!!")
        time.sleep(1)
        do_cmd(['python3', 'build_pass.py', '-c'], './')

    execute = True
    while execute:
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
                run_nas_cg_benchmark(args.save)
            case '5':
                run_randacc_benchmark()
            case 's':
                args.save = not args.save
            case 'e':
                execute = False
            case _:
                print("invalid option!")
        
        print("")
