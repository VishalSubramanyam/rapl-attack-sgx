import numpy as np
from extract import extract_readings
import sys
# first two args are the names of the two computations to be compared
comp1 = sys.argv[1]
comp2 = sys.argv[2]
computationPowerMap1, power1 = extract_readings(f'{comp1}.energy.csv', f'{comp1}.time.txt')
computationPowerMap2, power2 = extract_readings(f'{comp2}.energy.csv', f'{comp2}.time.txt')
# each map above contains one key-value pair, where key is the name of a computation, and the value is a tuple of two ints
# the first int is the starting slot number, and the second int is the ending slot number

# extract the power readings for the first computation
readings1 = computationPowerMap1[f'{comp1}']
# extract the power readings for the second computation
readings2 = computationPowerMap2[f'{comp2}']
# perform a t-test between the two lists of power readings
import scipy.stats as stats
print(f"t-test value: {stats.ttest_ind(readings1, readings2).statistic}")