#include "Enclave_u.h"
#include "computations.hpp"
#include "rapl.hpp"
#include "sgx_urts.h"
#include "utilities.h"
#include <chrono>
#include <climits>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <iterator>
#include <map>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
using clock_used = std::chrono::steady_clock;
using time_point = std::chrono::time_point<clock_used>;

int PINNED_CPU = 0;
int SAMPLING_INTERVAL = 150; // microseconds

bool victimDone = false;
bool spyMonitoringStarted = false;
time_point spyGraphStartTime;

void victimThread(std::vector<std::string> const &computationStrings,
                  sgx_enclave_id_t eid)
{
    set_affinity(PINNED_CPU);
    while (!spyMonitoringStarted)
        ; // spin until spy starts
    Computation::init(spyGraphStartTime, SAMPLING_INTERVAL, eid);
    remove("time_slots.txt");
    std::vector<Computation> computations;
    for (auto const &computationName : computationStrings) {
        std::cout << "DEBUG: Computation Name -> " << computationName
                  << std::endl;
        computations.emplace_back(computationParser.at(computationName)());
    }

    // execute every computation
    for (auto &comp : computations) {
        comp.performComputation();
    }
    // set victimDone to true
    victimDone = true;

    // print DONE
    printf("Victim DONE\n");
}

void spyThread()
{
    // pin this process to a different CPU than that of the victim to prevent
    // interference of any kind
    if (PINNED_CPU != 0) {
        set_affinity(0);
    } else {
        set_affinity(1);
    }
    // Monitor the energy consumption of the child process at regular intervals

    // Sampling interval
    std::chrono::microseconds interval(SAMPLING_INTERVAL);
    std::vector<double> energyConsumed;
    auto startTimeCurrentInterval = clock_used::now();
    spyGraphStartTime = startTimeCurrentInterval;
    while (true) {
        // check if child process has exited
        // TODO: Will the following if statement pollute the energy data?
        if (victimDone) {
            break;
        }
        // get energy consumed
        energyConsumed.push_back(getEnergyConsumed(PINNED_CPU));
        auto endTime = clock_used::now();
        // busy wait for the rest of the interval
        do {
            endTime = clock_used::now();
        } while (endTime - startTimeCurrentInterval < interval);
        startTimeCurrentInterval += interval;
        // if we were context switched out, insert -1 into energyConsumed for
        // every time slot missed
        while (endTime - startTimeCurrentInterval > interval) {
            energyConsumed.push_back(0);
            startTimeCurrentInterval += interval;
        }
        spyMonitoringStarted = true;
    }
    // output energy consumed to energy_readings.csv
    auto energyFile = fopen("energy_readings.csv", "w");
    for (auto &energy : energyConsumed) {
        fprintf(energyFile, "%lf\n", energy);
    }
}

int main(int argc, char **argv)
{
    // arg count check
    if (argc < 4) {
        printf(
            "Usage: %s <PINNED_CPU> <SAMPLING_INTERVAL> <COMPUTATION_NAMES (space-separated)>\n",
            argv[0]);
        // print out the arguments
        for (int i = 0; i < argc; i++) {
            printf("argv[%d] = %s\n", i, argv[i]);
        }
        exit(EXIT_FAILURE);
    }
    // argv[1] = PINNED_CPU
    PINNED_CPU = atoi(argv[1]);
    // argv[2] = SAMPLING_INTERVAL
    SAMPLING_INTERVAL = atoi(argv[2]);

    // argv[3..argc - 1] = computationNames
    std::vector<std::string> computationNames;
    for (int i = 3; i < argc; i++) {
        computationNames.push_back(argv[i]);
    }

    sgx_enclave_id_t eid;
    sgx_launch_token_t token;
    memset(&token, 0, sizeof(sgx_launch_token_t));
    int updated = 0;
    if (auto ret = sgx_create_enclave(
            "enclave.signed.so", SGX_DEBUG_FLAG, &token, &updated, &eid, NULL);
        ret != SGX_SUCCESS) {
        print_error_message(ret);
        return -1;
    }
    std::jthread victim(victimThread, computationNames, eid);
    spyThread();
    sgx_destroy_enclave(eid);
    return 0;
}