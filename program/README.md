# Benchmarks
This folder contains all the benchmarks we used to generate all data in our project. The following section describes the general folder structure for each benchmark.  
  
Please reference the original paper for detailed explanations of what each benchmark does:  
  
Sam Ainsworth and Timothy M. Jones. 2017. Software prefetching for indirect  
memory accesses. , 305-317 pages. https://doi.org/10.1109/CGO.2017.7863749

# Requirements
It is expected that all the `generate_ipc.sh` scripts are run before the pass is built.  
  
It is expected that the pass is built before the `compile_x86.sh` or `print_computed_c_val.sh` are ran  
  
**Note:** The top level python script will handle this for you.

# Folder Structure
Each benchmark folder has all the source code required to generate the benchmark and what is described in the following sections.

## legal/
This folder contains any legal files that came with the benchmark. This includes, but is not limited to, copyright and license notices.

## generate_ipc.sh
This script is used to estimate the Instruction-Per-Cycle for this benchmark. It will output the estimated value to a `value.txt` file at the root of the project.

**Note:** The order that these estimates gets generated and entered into `value.txt` is extremely important. 
We do not recommend manually running these scripts, but rather using the top level python script to handle this for you. 
However, if you still want to manually run these scripts, you can run them using the alpabetical order of the top level folder (i.e. graph500, hashjoin-ph2, etc.)

## compile_x86.sh
This script generates 3 binaries under `test/bin/x86` for the benchmark where the postfix represents the following:
- `-no` = This is the benchmark with no prefetching
- `-auto` = This is the benchmark with the original prefetch with a hard coded C value
- `-auto-new` = This is the benchmark with the modified prefetch with a dynamically computed C value 

## print_computed_c_val.sh
This script will output all debug output from our prefetch pass. It will display the computed C value in the following format:  
  
`Calculated C Const Value: X ... will be cast to Y`  
  
Where X is a `double` representation of what is calculated, and Y is the value after it is casted to an `int`.
