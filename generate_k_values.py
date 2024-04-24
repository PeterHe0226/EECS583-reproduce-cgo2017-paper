#!/usr/bin/env python3
import numpy as np
import json
import pathlib

datafile = pathlib.Path('./data.json')

# Load JSON data from file
with open(datafile, 'r') as file:
    data = json.load(file)

# Extract matrix A and vector B
A = np.array([[entry['clock_speed'], entry['cpu_cores'], entry['cache_size'],
               entry['ram_size'], entry['page_size']] for entry in data])
B = np.array([entry['optimal_c_value'] for entry in data])

# Using np.linalg.lstsq to find the least squares solution if the system is overdetermined or singular
X, residuals, rank, s = np.linalg.lstsq(A, B, rcond=None)

# Print the solution for X
print("Solution for X:", X)
if residuals.size > 0:
    print("Residuals:", residuals)
print("Rank of matrix A:", rank)
print("Singular values:", s)


modified = False
filename = './freshAttempt/swPrefetchPass/swPrefetchPass.cpp'
new_values = ','.join(X.astype(str))
# Open the original file in read mode
lines = []
with open(filename, 'r') as file:
    for line in file:
        # Check if the line contains the desired declaration
        if line.strip().startswith('const std::array<double, 5> K_VALUES'):
            # Find the curly brackets
            start = line.find('{')
            end = line.find('}', start)
            if start != -1 and end != -1:
                # Replace the content between the curly brackets with new values
                line = line[:start+1] + new_values + line[end:]
                modified = True
        # Append the possibly modified line to the list
        lines.append(line)

# Only write back if a modification was made
if modified:
    # Write the modified lines back to the file
    with open(filename, 'w') as file:
        file.writelines(lines)