#pragma once
#include <openssl/aes.h>

#include <chrono>
#include <functional>
#include <map>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include "sgx_urts.h"

using clock_used = std::chrono::steady_clock;
using time_point = std::chrono::time_point<clock_used>;

extern "C" {
extern void emptyComputation();
extern void aesOpenSSLComputation();
extern void avx2Computation();
extern void mixedSIMDComputation();
extern void aesniComputation();
extern void aesniKeyFixedPtFixed();
extern void aesniKeyVariesPtFixed();
extern void aesOpenSSLKeyFixedPtFixed();
extern void aesOpenSSLKeyVariesPtFixed();
extern void aesOpenSSLKeyFixedPtFixedSGX();
extern void aesOpenSSLKeyVariesPtFixedSGX();
extern void aesOpenSSLKeyFixedPtFixedLoopedSGX();
extern void multiplyFull();
extern void multiplyHalf();
extern void multiplyQuarter();
}

class Computation
{
    static inline time_point spyStartTime;
    static inline uint samplingInterval;
    static inline bool isInitialized = false;
    static inline sgx_enclave_id_t eid;
    uint startTimeSlot;
    uint endTimeSlot;
    std::string name;

  public:
    static sgx_enclave_id_t get_eid() { return eid; }
    void (*compFuncPtr)(void);
    static void init(time_point const &spyStart, uint samplingInt,
                     sgx_enclave_id_t eid);
    /**
     * @brief Construct a new Computation object
     *
     * @param name Name of the computation
     * @param func Function pointer to the computation
     * @param spyStartTime Time point when the spy thread started recording the
     * energy values
     * @param samplingInterval Sampling interval in microseconds
     */
    Computation(std::string name, void (*func)(void))
    {
        if (!isInitialized) {
            throw std::runtime_error("Computation class not initialized");
        }
        this->name = name;
        this->compFuncPtr = func;
    }
    void performComputation()
    {
        printf("Starting computation %s\n", this->name.c_str());
        time_point startTime = clock_used::now();
        this->compFuncPtr();
        time_point endTime = clock_used::now();
        // convert to slots based on spyStartTime and samplingInterval
        this->startTimeSlot =
            std::chrono::duration_cast<std::chrono::microseconds>(startTime -
                                                                  spyStartTime)
                .count() /
            samplingInterval;
        this->endTimeSlot =
            std::chrono::duration_cast<std::chrono::microseconds>(endTime -
                                                                  spyStartTime)
                .count() /
            samplingInterval;
        // open time_slots.txt in append mode
        FILE *fp = fopen("time_slots.txt", "a");
        fprintf(fp, "%s %d %d\n", this->name.c_str(), this->startTimeSlot,
                this->endTimeSlot);
        fclose(fp);
        printf("Ending computation %s\n", this->name.c_str());
        fflush(stdout);
    }
};
extern std::unordered_map<std::string, std::function<Computation(void)>> const
    computationParser;