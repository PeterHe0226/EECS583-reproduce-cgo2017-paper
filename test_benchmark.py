import re
import sys
import csv
import argparse
import subprocess
import pathlib
import numpy as np

def parser_create():
    parser = argparse.ArgumentParser(description='start c constant value and increment value')
    parser.add_argument('--c', type=int, default=4, help='start constant c value to use')
    parser.add_argument('--i', type=int, default=2, help='the power value of c value to increase each time')
    parser.add_argument('--s', type=int, default=5, help='the time to increment the c value')
    return parser


def get_args():
    global parser
    parser = parser_create()
    args = parser.parse_args()

    return args

def modify_cpp_constant(file_path, new_value):

    pattern = re.compile(r'^#define C_CONSTANT \(\d+\)$')

    with open(file_path, 'r') as file:
        lines = file.readlines()

    
    with open(file_path, 'w') as file:
        for line in lines:
            if pattern.match(line.strip()):
                # Replace the existing value with the new value
                line = f'#define C_CONSTANT ({new_value})\n'
            file.write(line)


# modify_cpp_constant(pathlib.Path('./freshAttempt/swPrefetchPass/swPrefetchPass.cpp'), 88)

def run_script_with_inputs(script_name, initial_input, exit_input, start_input):

    process = subprocess.Popen(['python3', script_name], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)

    stdout, stderr = process.communicate(input= start_input + '\n' + initial_input + '\n' + exit_input + '\n')
    # print("Output after initial input:", stdout)
    # if stderr:
    #     print("Error:", stderr)

    # # Check if process is still running, if not, no need to send 'e'
    # if process.poll() is None:
    #     process.stdin.write(exit_input + '\n')
    #     process.stdin.flush()

    # # process.stdin.close()

    return_code = process.wait()
    if return_code == 0:
        print("Script executed successfully")
    else:
        print("Script exited with error")
    
    return stdout
    
# run_script_with_inputs('test_and_benchmark.py', '4', 'e', 'p')

def catch_time_nas_cg(output):
    time_pattern = re.compile(r"Time in seconds =\s+([\d\.]+)")
    times = time_pattern.findall(output)

    # print(type(times))
    return times


def write_to_csv(file_path, data):
    with open(file_path, mode='w', newline='') as file:
        writer = csv.writer(file)
        
        writer.writerow(['Constant','no', 'auto', 'auto-new'])
        
        for row in data:
            writer.writerow(row)

def main():

    args = get_args()

    constant_val = args.c
    increment_val = args.i
    maximum_to_increment = args.s
    
    file_path = pathlib.Path('./freshAttempt/swPrefetchPass/swPrefetchPass.cpp')

    exit_option = 'e' # exit for the program
    build_option = 'b' # rebuild the pass and all benchmarks
    test_option = '4' # which test to go, option 4 here is nas-cg
    test_script_name = 'test_and_benchmark.py'
    mutiple_times = 5 # how many time to repeat 
    time_list = []

    for _ in range(maximum_to_increment):

        average_list = np.array([0, 0, 0], dtype=np.float32)
        constant_val = constant_val * increment_val
        modify_cpp_constant(file_path, constant_val)

        for _ in range(mutiple_times):
            output = run_script_with_inputs(test_script_name,test_option, exit_option,build_option)
            if test_option == '4':
                output = catch_time_nas_cg(output)
            average_list = average_list + np.array(output).astype(float)


        average_list = (average_list / mutiple_times).tolist()
        print(average_list)
        run_time = [constant_val]
        run_time.extend(average_list)
        
        time_list.append(run_time)

    
    write_to_csv(f'test_benchmark_{test_option}.csv',time_list)


main()