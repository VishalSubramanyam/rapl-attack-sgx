#!/usr/bin/env python3
# read input file from first argument
# Each line contains one number
# Read each number into a list

import sys
import math
import numpy as np
# Read input file
if len(sys.argv) != 3:
    print("Usage: python3 energy_power.py <energy file> <time file>")
    sys.exit(1)

# Read input file
energy_file = sys.argv[1]
time_file = sys.argv[2]
from extract import extract_readings

computationPowerMap, power = extract_readings(energy_file, time_file)

# Compute the mean, median of the power readings for each computation in from start to end
# Construct a dictionary of computation names and their mean and median power readings
# The dictionary is of the format:
#       computation_name: [mean, median]
computationStats = {}
for computationName in computationPowerMap:
    computationPower = computationPowerMap[computationName]
    mean = np.mean(computationPower)
    median = np.median(computationPower)
    # Add the mean and median to the dictionary
    computationStats[computationName] = (mean, median)

# Print the mean and median power readings for each computation with proper labels
for computation in computationStats:
    print(computation + ":")
    print("Mean: " + str(computationStats[computation][0]))
    print("Median: " + str(computationStats[computation][1]))
    print()

# Each power reading is made after an interval of "sampling_interval" microseconds
# Plot power readings against time, noting that power list indices don't indicate time
# Construct time list

# Plot power readings against time
import matplotlib.pyplot as plt
import matplotlib
matplotlib.use('TKagg')
# Plot the graph with red markers on actual datapoints and lines connecting the markers
# should be in black
plt.plot(power, 'ro', power, 'k')
#plt.plot(time, power)
plt.xlabel("Time (slots)")
plt.ylabel("Power (Joules / timeSlot)")
plt.title("Power vs Time")
#plt.savefig("powerChart.png")
plt.show()

# Plot the frequency chart of power readings for each computation
for computationName in computationPowerMap:
    computationPower = computationPowerMap[computationName]
    # Plot the frequency chart of power readings for the computation
    plt.hist(computationPower, bins=200, density=True)
plt.xlabel("Power (Joules / timeSlot)")
plt.ylabel("Probability Density")
plt.title("Probability density chart of Power Readings")
# legend
plt.legend(computationPowerMap.keys())
#plt.savefig("probChart.png")
plt.show()
