#ifndef UTILS_H
#define UTILS_H

#include <vector>
#include <string>
#include <sys/types.h>
#include <signal.h>
#include <cstdint>

#include "config.h"

enum Colors
{
        NONE = 0,
        RED,
        GREEN,
        YELLOW,
        BLUE,
        MAGENTA,
        CYAN,
        ORANGE,
};

const std::string colors[] = {
        "\033[0m",
        "\033[31m",
        "\033[32m",
        "\033[33m",
        "\033[34m",
        "\033[35m",
        "\033[36m",
        "\033[91m",
};

enum ProcessTypes
{
        PROCESS_MAIN = 0,
        PROCESS_BAGGAGE_CONTROL,
        PROCESS_SECURITY_CONTROL,
        PROCESS_STAIRS,
        PROCESS_DISPATCHER,
};

enum FIFOs
{
        FIFO_BAGGAGE_CONTROL = 0, // INFO: FIFO for communication between baggage control and passengers
        FIFO_SECURITY_CONTROL, // INFO: FIFO for communication between security control and passengers
        FIFO_SECURITY_SELECTOR, // INFO: FIFO for communication between security selector and passengers
        FIFO_SECURITY_GATE_0, // INFO: FIFO for communication between security gate 0 and passengers
        FIFO_SECURITY_GATE_1, // INFO: FIFO for communication between security gate 1 and passengers
        FIFO_SECURITY_GATE_2, // INFO: FIFO for communication between security gate 2 and passengers
};

const std::string fifoNames[] = {
        "baggageControlFIFO",
        "secControlFIFO",
        "secSelectorFIFO",
        "secGate0FIFO",
        "secGate1FIFO",
        "secGate2FIFO",
};

enum Semaphores
{
        SEM_BAGGAGE_CONTROL_ENTRANCE = 0, // INFO: semaphore for queue in front of baggage control
        SEM_BAGGAGE_CONTROL_OUT, // INFO: semaphore for synchronization after baggage control
        SEM_SECURITY_CONTROL_ENTRANCE,  // INFO: semaphore for queue in front of security control
        SEM_SECURITY_CONTROL_ENTRANCE_WAIT, // INFO: semaphore for synchronization after security control entrance
        SEM_SECURITY_CONTROL_SELECTOR_ENTRANCE_WAIT, // INFO: semaphore for synchronization after security control selector entrance
        SEM_SECURITY_CONTROL_SELECTOR_WAIT, // INFO: semaphore for synchronization after security control selector
        SEM_SECURITY_GATE_0, // INFO: semaphore queue in front of security gate 0
        SEM_SECURITY_GATE_1, // INFO: semaphore queue in front of security gate 1
        SEM_SECURITY_GATE_2, // INFO: semaphore queue in front of security gate 2
        SEM_SECURITY_GATE_0_WAIT, // INFO: semaphore for synchronization after security gate 0
        SEM_SECURITY_GATE_1_WAIT, // INFO: semaphore for synchronization after security gate 1
        SEM_SECURITY_GATE_2_WAIT, // INFO: semaphore for synchronization after security gate 2
        SEM_SECURITY_CONTROL_OUT, // INFO: semaphore for synchronization after security control
        SEM_STAIRS_PASSENGER_IN, // INFO: semaphores for synchronization when passenger enters stairs
        SEM_STAIRS_PASSENGER_WAIT,
        SEM_PLANE_PASSANGER_IN, // INFO: semaphores for synchronization when passenger enters plane
        SEM_PLANE_PASSANGER_WAIT,
        SEM_STAIRS_COUNTER, // INFO: semaphore for counting passengers on stairs
        SEM_PLANE_COUNTER, // INFO: semaphore for counting passengers on plane
        SEM_PASSENGER_COUNTER, // INFO: semaphore for counting passengers that finished their tasks
        SEM_PLANE_WAIT, // INFO: semaphore for synchronization after plane is ready to depart
        SEM_STAIRS_WAIT, // INFO: semaphore for synchronization after stairs are ready to close
        SEM_PLANE_DEPART, // INFO: semaphore for synchronization between dispatcher and plane
        SEM_SECURITY_CONTROL_SELECTOR, // INFO: group of semaphores used to indicate specific passenger entering security selector
};

enum Signals
{
        SIGNAL_PASSENGER_IS_OVERWEIGHT = 35,
        SIGNAL_PASSENGER_IS_DANGEROUS,
        SIGNAL_PASSENGER_SKIPPED,
        SIGNAL_STAIRS_CLOSE,
        SIGNAL_PLANE_GO,
        SIGNAL_PLANE_READY, // INFO: signal from plane to dispatcher, indicates that plane arrived
        SIGNAL_PLANE_READY_DEPART, // INFO: signal from plane to dispatcher, indicates that plane is ready to depart
        SIGNAL_DISPATCHER_PLANE_FORCED_DEPART, // INFO: signal from dispatcher to plane, indicates that plane must depart
        SIGNAL_THIS_IS_THE_END_HOLD_YOUR_BREATH_AND_COUNT_TO_TEN, // INFO: ends the program
        SIGNAL_OK, // INFO: placeholder signal for testing
};

enum LOGS
{
        LOG_NONE = 0,
        LOG_PASSENGER,
        LOG_BAGGAGE_CONTROL,
        LOG_SECURITY_CONTROL,
        LOG_STAIRS,
        LOG_DISPATCHER,
        LOG_PLANE,
        LOG_MAIN,
};

/*
* function initializing semaphores
*/
std::vector<int> initSemaphores(int permissions, size_t numPassengers);

/*
* functions for generating random values
*/
void genRandomVector(std::vector<uint64_t> &vec, uint64_t min, uint64_t max);
void genRandom(uint64_t &val, uint64_t min, uint64_t max);

/*
* function for printing when verbose is enabled
*/
void vCout(const std::string &msg, int color = 0, int log = 0);

/*
* functions for creating named processes
*/
void createSubprocesses(size_t n, std::vector<pid_t> &pids, const std::vector<std::string> &names);
void createSubprocess(pid_t &pid, const std::string &name);

/*
* functions for safely handling fifos
*/
void safeFIFOOpen(int &fd, const std::string &name, int flags);
void safeFIFOWrite(int fd, const void *buf, size_t size);
void safeFIFORead(int fd, void *buf, size_t size);
void safeFIFOClose(int fd);

/*
* functions for safely handling semaphores
*/
void safeSemop(int semID, struct sembuf *sops, size_t nsops);
int safeGetSemVal(int semID, int semnum);
void safeSetSemVal(int semID, int semnum, int val);

#endif // UTILS_H
