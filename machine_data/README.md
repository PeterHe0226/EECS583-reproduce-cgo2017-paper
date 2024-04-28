# Machine Data
This folder houses json files used to generate K Values for both the eecs 583 servers made available to students (server A and B).  

Each file contains machine information from the four papers in the original paper. Furthermore, each machine dependent json will contain machine information on the other eecs 583 machine.  

This is done intentionally because it simulates tuning K Values for a new machine based on data from other machines.

# Layout
`machine` - machine that the entry represents  
  
`clock_speed` - cpu clock speed measured in GHz  
  
`cpu_cores` - number of physical cpu cores (does not factor for num threads per core)  
  
`cache_size` - L1 cache size (Kb)  
  
`ram_size` - measured in Kb  
  
`page_size` - kernel page size (Kb)  
  
`optimal_c_value` - optimal c value found when sweeping across multiple values  
