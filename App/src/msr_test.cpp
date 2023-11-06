#include "msr.hpp"
#include <iostream>
using namespace std;
#include <unistd.h>
#include <fcntl.h>
#include "rapl.hpp"
int main(){
    // set affinity to cpu 10
    set_affinity(10);

    // read some MSR value
    auto fd = open("/dev/msr_driver", O_RDONLY);
    if (fd == -1) {
        auto error = strerror(errno);
        cout << "Could not open MSR kernel interface" << endl;
        cout << "Error: " << error << endl;
        return 1;
    }
    uint64_t msr_value;
    auto retVal = pread(fd, &msr_value, sizeof(msr_value), 0x00000639);
    cout << "retVal: " << retVal << endl;
    if (retVal != sizeof(msr_value)) {
        auto error = strerror(errno);
        cout << "Could not read MSR register" << endl;
        cout << "Error: " << error << endl;
        return 1;
    }
    cout << "MSR register value: " << msr_value << endl;
}