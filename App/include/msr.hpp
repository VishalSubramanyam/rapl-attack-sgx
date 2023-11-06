#pragma once
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdexcept>
#include <vector>
namespace MSR {
// extracted from:
// elixir.bootlin.com/linux/latest/source/arch/x86/include/asm/msr-index.h
enum {
    MSR_VR_CURRENT_CONFIG = 0x00000601,
    MSR_RAPL_POWER_UNIT = 0x00000606,

    MSR_PKG_POWER_LIMIT = 0x00000610,
    MSR_PKG_ENERGY_STATUS = 0x00000611,
    MSR_PKG_PERF_STATUS = 0x00000613,
    MSR_PKG_POWER_INFO = 0x00000614,

    MSR_DRAM_POWER_LIMIT = 0x00000618,
    MSR_DRAM_ENERGY_STATUS = 0x00000619,
    MSR_DRAM_PERF_STATUS = 0x0000061b,
    MSR_DRAM_POWER_INFO = 0x0000061c,

    MSR_PP0_POWER_LIMIT = 0x00000638,
    MSR_PP0_ENERGY_STATUS = 0x00000639,
    MSR_PP0_POLICY = 0x0000063a,
    MSR_PP0_PERF_STATUS = 0x0000063b,

    MSR_PP1_POWER_LIMIT = 0x00000640,
    MSR_PP1_ENERGY_STATUS = 0x00000641,
    MSR_PP1_POLICY = 0x00000642
};

class MSRreader {
   private:
    // maintain a list of fds for each core
    std::vector<int> fd;

   public:
    MSRreader() {
        // for every core, open the MSR kernel interface
        for (int i = 0; i < sysconf(_SC_NPROCESSORS_CONF); i++) {
            std::string msrInterfacePath =
                "/dev/cpu/" + std::to_string(i) + "/msr";
            int fd = open(msrInterfacePath.c_str(), O_RDWR);
            if (fd == -1)
                throw std::runtime_error(
                    std::string("Could not open MSR kernel interface at ") +
                    msrInterfacePath
                );
            this->fd.push_back(fd);
        }
    }
    /**
        * @brief Reads the MSR register using the given hex notation for that particular MSR
        *        for the given CPU
        * 
        * @param msr The address of the MSR register to read
        * @param cpu The CPU to read from
        * @return uint64_t The value of the MSR register
    */
    uint64_t readMSR(off_t msr, int cpu) {
        // bounds checking
        if (cpu < 0 || cpu >= this->fd.size())
            throw std::runtime_error(
                std::string("readMSR() failed: CPU ") + std::to_string(cpu) +
                " does not exist"
            );
        uint64_t returnVal;
        auto errorCode = pread(fd[cpu], &returnVal, sizeof(returnVal), msr);
        if (errorCode == 0)
            throw std::runtime_error(
                "readMSR() failed: Kernel interface has returned EOF"
            );
        else if (errorCode == -1)
            throw std::runtime_error(strerror(errno));
        else
            return returnVal;  // nothing to do
    }
};
};  // namespace MSR