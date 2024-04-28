# Overview
This folder houses our implementation of the original prefetch pass in llvm 17 with improvements to code quality (tabs/spaces, readability, c++ best practices, etc.). Furthermore, there is a demo folder which contains a small example to see that our implementation works and inserts prefetches.

# Requirements
Building the pass requires that the ipc estimate file has been generated. We recommend using the top level python script to handle this for you. However, if you would like to build manually, pleas follow instruction under program/ on how to generate the ipc estimate file, and then use cmake to build.

# Build System
We use a standard cmake build system to generate shared libs of our llvm pass, however, we define multiple targets so that we have a specific shared lib for each of the benchmarks used. 
This was done because we have shell scripts which estimate instructions per cycle (IPC) for a specific program, and these estimates need to be read into our pass when the pass is running.
We make use of preprocessor definitions via the build command to make all of this work.

## Preprocessor Definitions
The following subsections go over how each preprocessor definition is used.

### COMPUTE_C_CONST
When this preprocessor definition is defined we disable usage of the hard coded C constant and enable our dynamic computation instead.  

Compiling without this flag builds the original pass that the original authors implemented with the hardcoded C constant.

### IPC_INDEX
This preprocessor definition is used when the COMPUTE_C_CONST definition is defined, and it indicates which line in the ipc estimate file the benchmark specific estimate is located.
