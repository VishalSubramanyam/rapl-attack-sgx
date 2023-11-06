#!/usr/bin/env bash
SCRIPTS_FOLDER=scripts
# Check required command line args
if [ "$#" -le 3 ]; then
    echo "Illegal number of parameters"
    echo "Usage: ./run.sh <logical cpu> <sampling interval> <t_test/t_test_separate/graph/cpa> <computations...>"
    exit 1
fi
LOGICAL_CPU=$1
SAMPLING_INTERVAL=$2
MODE=$3

# Check if venv exists
if [ ! -d "venv" ]; then
    # Create venv if it doesn't exist
    python3 -m venv venv
    # install necessary python3 packages
    source venv/bin/activate
    pip install matplotlib
    # install scipy
    pip install scipy
    # Check if python3-tk is installed (assuming Ubuntu)
    if ! dpkg -s python3-tk >/dev/null 2>&1; then
        # Install python3-tk if it isn't installed
        sudo apt-get install -y python3-tk
    fi
fi
source venv/bin/activate
source $HOME/subramanyam/environment

# Cmake should use a specific compiler kit
export CC=gcc-10
export CXX=g++-10

# Make build directory if it doesn't exist
mkdir -p build

# Use Cmake to build the project into build dir
cd build
cmake ..
cmake --build .
cd ..

# Run the executable
# Format: ./build/rapl_threaded <logical cpu> <sampling interval>
# logical cpu: The cpu to pin the victim process
# sampling interval: The interval in microseconds to sample the power consumption
# Take the arguments from the first two arguments of the script
# Check exit code of the executable run with sudo
# if ! sudo ./build/rapl_threaded $1 $2 empty,aesniKeyFixedPtVaries,empty,aesniPtFixedKeyVaries; then
#     # Print error message
#     echo "Error running rapl_threaded"
#     exit 1
# fi
cd build
if [[ $MODE == "t_test_separate" ]]; then
    # check that there are only 2 computations given
    if [[ $# -ne 5 ]]; then
        echo "Expected 2 computations"
        exit 1
    fi
    for((i=4; i<=$#; i++)); do
        arg="${!i}"
        # adding empty computation to ensure initial power variations are ignored
        sudo ./rapl_attack $1 $2 empty $arg
        sudo mv energy_readings.csv $arg.csv
        sudo mv time_slots.txt $arg.time.txt
    done
elif [[ $MODE == "graph_separate" ]]; then
    # check that there are at least 2 computations given
    if [[ $# -lt 5 ]]; then
        echo "Expected at least 2 computations"
        exit 1
    fi
    for((i=4; i<=$#; i++)); do
        arg="${!i}"
        # adding empty computation to ensure initial power variations are ignored
        sudo ./rapl_attack $1 $2 empty $arg
        sudo mv energy_readings.csv $arg.energy.csv
        sudo mv time_slots.txt $arg.time.txt
    done
else
    # set arg to the concat of all arguments starting with the 4th argument
    arg="${*:4}"
    sudo ./rapl_attack $1 $2 $arg
fi
# Plot the results
if [[ $MODE == "graph" ]]; then
    python3 ../$SCRIPTS_FOLDER/energy_power.py energy_readings.csv time_slots.txt
elif [[ $MODE == "t_test" ]]; then
    comp1="${@:4:1}"
    comp2="${@:5:1}"
    python3 ../$SCRIPTS_FOLDER/t_test.py energy_readings.csv time_slots.txt $comp1 $comp2
elif [[ $MODE == "t_test_separate" ]]; then
    comp1="${@:4:1}"
    comp2="${@:5:1}"
    python3 ../$SCRIPTS_FOLDER/t_test_separate.py $comp1 $comp2
elif [[ $MODE == "cpa" ]]; then
    python3 ../$SCRIPTS_FOLDER/cpa.py energy_readings.csv
# adding a new mode called "graph_separate"
elif [[ $MODE == "graph_separate" ]]; then
    # pass all the computation names to the python script.
    # computation names start from the 4th argument
    python3 ../$SCRIPTS_FOLDER/energy_power_separate.py ${*:4}
else
    echo "Invalid argument - Expected t_test/t_test_separate/graph/cpa"
    exit 1
fi
