#pragma once
#include <sched.h>
#include "msr.hpp"
// returns energy consumed in joules
inline double getEnergyConsumed(int core) {
    static MSR::MSRreader msrReader;
    // extracts bits 12:8, core doesn't matter for Energy Status Unit
    static auto ESU =
        msrReader.readMSR(MSR::MSR_RAPL_POWER_UNIT, core) >> 8 & 0b11111;
    static double unitMultiplier = 1.0 / (1 << ESU);
    return (msrReader.readMSR(MSR::MSR_PP0_ENERGY_STATUS, core) & 0xFFFFFFFF) *
           unitMultiplier;
}

/**
    * @brief Sets the affinity of the current process to the given logical CPU
    * 
    * @param cpu The CPU to pin the current process to
*/
inline void set_affinity(int cpu){
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(cpu, &mask);
    sched_setaffinity(0, sizeof(mask), &mask);
}