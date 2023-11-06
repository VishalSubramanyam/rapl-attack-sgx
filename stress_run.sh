#!/bin/bash
# Run run.sh 0 1800 t_test a thousand times and record the t_test value from each iteration
# The t-test value is presented on the last line of output of each execution in the format:
# t-test value: <t-test value>
# The t-test value is extracted using "tail" and "cut" and stored in a file called t_test_values.txt
# Perform some statistical stuff on the t-test values like mean, median, standard deviation and print them
# Perform statistical stuff using python3

for i in {1..30}
do
    ./run.sh 0 1800 t_test aesOpenSSLKeyFixedPtFixedLoopedSGX aesOpenSSLKeyVariesPtFixedLoopedSGX | tail -n 1 | cut -d " " -f 3 >> t_test_values.txt
done

python3 -c "import statistics; import math; import numpy as np; t_test_values = np.loadtxt('t_test_values.txt'); print('Mean: ' + str(statistics.mean(t_test_values))); print('Median: ' + str(statistics.median(t_test_values))); print('Standard Deviation: ' + str(statistics.stdev(t_test_values))); print('Standard Error: ' + str(statistics.stdev(t_test_values)/math.sqrt(len(t_test_values))));"
