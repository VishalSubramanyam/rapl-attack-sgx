#!/usr/bin/env python3
# read input file from first argument
# Each line contains one number
# Read each number into a list
import sys
# Read input file
if len(sys.argv) != 5:
    print("Usage: python3 t_test.py <energy file> <time file> <computation1_name> <computation2_name>")
    sys.exit(1)

energy_file = sys.argv[1]
time_file = sys.argv[2]
computation1_name = sys.argv[3]
computation2_name = sys.argv[4]
# Perform a t-test between the power readings of the first computation and the second computation
# Power readings are available in the power list
# The start and end time slots for the first computation are available in the computation_times dictionary
import scipy.stats as stats
from extract import extract_readings
computationPowerMap, power = extract_readings(energy_file, time_file)
print(f"t-test value: {stats.ttest_ind(computationPowerMap[computation1_name], computationPowerMap[computation2_name]).statistic}")
