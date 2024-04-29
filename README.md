# Overview
This repo houses all code and scripts written by Group 17 for EECS 583 Winter 2023. This readme has information pertaining to the python scripts at the root folder, and how to get started with testing/benchmarking. Open subfolders to find readmes with more context-specific information. Below are links to all the other readmes:  

- [Prefetch LLVM 17 Pass](freshAttempt/README.md)  
- [Simple Demo](freshAttempt/demo/README.md)  
- [Benchmarks](program/README.md)  
- [Optimal C Values Results](optimal_c_value/README.md)  
- [Machine HW Information](machine_data/README.md)  
- [Legal Information](legal/README.md)  

**Note:** The Getting Started is at the bottom since it is expected that the user goes through the background section to understand how to use the scripts
# Background
This section will go over the purpose of both python scripts and how to use them.  

## generate_k_values.py
Usage:  
`python3 generate_k_values.py -X`  
Where X = `a` or `b` depending on if you are on server A or B.  

This script uses the machine specific json file under machine data to generate K Values as discussed in our paper. This script will also automatically update the K Values std::array in the swPrefetchPass.cpp, so no manual copy/pasting is needed.

## test_and_benchmark.py
This script is responsible for automating all aspects of development and testing for our project.  

Usage:  
`python3 test_and_benchmark.py`  

The first time this script is ran it will generate estimates for instruction-per-cycle for all benchmarks, build all targets of our llvm pass, and build and link all benchmarks against the appropriate pass.  

Below is the menu it will display:  
`a - run all benchmarks`  
`b - rebuild the pass and all benchmarks`  
`f - find optimal c value`  
`c - print computed c values for benchmarks`  
`1 - run graph500  benchmark`  
`2 - run hashjoin2 benchmark`  
`3 - run hashjoin8 benchmark`  
`4 - run nas-cg    benchmark`  
`5 - run randacc   benchmark`  
`s - toggle benchmark output save (currently OFF)`  
`e - exit`  
`>>`

The following sections go over what each option does.

### a
This will run each benchmark 10 times and generate average execution time for binaries which don't use prefetching, use the original authors' prefetching, and our prefetching with dynamic c constant calculation.
It will concatenate all average-related output generated by individual benchmarks and print it out to the terminal after all tests have been ran. Look at the options that run individual benchmarks to see output style.
### b
This option will regenerate estimates for instruction-per-cycle for all benchmarks, build all targets of our llvm pass, and build and link all benchmarks against the appropriate pass. This option is used when you make a change to any build script or source code of the ipc generation, llvm pass, or benchmark.
### f
This feature will sweep through multiple C constant values to help a user determine which C constant is best for their machine. We used this option to automate the sweep process to determine the optimal c value for a machine which got fed into our `generate_k_values.py` script.
It is currently configured to start with C=32 and go up to C=256 incrementing by 32 on each iteration.  

Each iteration will automatically change the hard coded value in `freshAttempt/swPrefetchPass/swPrefetchPass.cpp`, rebuild pass/benchmarks, and run all benchmarks only on the binary compiled against the hard coded C const. Like the run all benchmarks option, on each iteration, it will run each benchmark 10 times and compute average execution time.  

At the end of all iterations it will populate a csv where each row is the average time for a specific benchmark for each hard coded C value that was tested. An example of this output is under program `optimal_c_value/machine_X/optimal_c_value_out.csv`. Furthermore, it will also generate a graph like this: `optimal_c_value/machine_X/machine_a_c_const_value.png`.
### c
This option will print out what dynamically computed C constant each benchmark is running with. The output will match the following (XX.XXX is a 3 decimal place floating point number):  
  
`*********************************************************`

`Benchmark graph500 computed c value = [XX.XXX]`

`Benchmark hashjoin-ph-2 computed c value = [XX.XXX]`

`Benchmark hashjoin-ph-8 computed c value = [XX.XXX]`

`Benchmark nas-cg computed c value = [XX.XXX]`

`Benchmark randacc computed c value = [XX.XXX]`

`*********************************************************`

### 1
This will run the graph500 benchmark 10 times and generate average execution time for binaries which don't use prefetching, use the original authors' prefetching, and our prefetching with dynamic c constant calculation. The average-related output for this test will look like below (XX.XX is a floating point number):  
  
`*********************************************************`  
`Benchmark:    graph500`  
`Normal:       XX.XX`  
`Prefetch:     XX.XX`  
`New Prefetch: XX.XX`  
`*********************************************************`
### 2
This will run the hashjoin2 benchmark 10 times and generate average execution time for binaries which don't use prefetching, use the original authors' prefetching, and our prefetching with dynamic c constant calculation. The average-related output for this test will look like below (XX.XX is a floating point number):  
  
`*********************************************************`  
`Benchmark:    hashjoin-ph-2`  
`Normal:       XX.XX`  
`Prefetch:     XX.XX`  
`New Prefetch: XX.XX`  
`*********************************************************`
### 3
This will run the hashjoin8 benchmark 10 times and generate average execution time for binaries which don't use prefetching, use the original authors' prefetching, and our prefetching with dynamic c constant calculation. The average-related output for this test will look like below (XX.XX is a floating point number):  
  
`*********************************************************`  
`Benchmark:    hashjoin-ph-8`  
`Normal:       XX.XX`  
`Prefetch:     XX.XX`  
`New Prefetch: XX.XX`  
`*********************************************************`
### 4
This will run the nas-cg benchmark 10 times and generate average execution time for binaries which don't use prefetching, use the original authors' prefetching, and our prefetching with dynamic c constant calculation. The average-related output for this test will look like below (XX.XX is a floating point number):  
  
`*********************************************************`  
`Benchmark:    nas-cg`  
`Normal:       XX.XX`  
`Prefetch:     XX.XX`  
`New Prefetch: XX.XX`  
`*********************************************************`
### 5
This will run the randacc benchmark 10 times and generate average execution time for binaries which don't use prefetching, use the original authors' prefetching, and our prefetching with dynamic c constant calculation. The average-related output for this test will look like below (XX.XX is a floating point number):  
  
`*********************************************************`  
`Benchmark:    randacc`  
`Normal:       XX.XX`  
`Prefetch:     XX.XX`  
`New Prefetch: XX.XX`  
`*********************************************************`
### s
Using this option toggles the feature to save program output for benchmarks. When enabled, the script will only save the output of the first repition of a specific benchmark instead of saving information from each repititon each time the benchmark is ran. It saves all output under the following folder:  
./benchmark_output  

The state of whether the script is saving files or not is displayed in the menu shown above.
### e
This option will stop running the input loop and exit from the program.


# Getting Started
Clone this repo and run the following commands from the repo root folder:  

`python3 generate_k_values.py -X`  
Replace X with either `a` or `b` based on which server you are on.  

`python3 test_and_benchmark.py`  
Use the options available based on the descriptions above to do what you want to
