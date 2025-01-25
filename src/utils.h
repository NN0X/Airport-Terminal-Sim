#ifndef UTILS_H
#define UTILS_H

#include <vector>
#include <string>
#include <sys/types.h>
#include <signal.h>
#include <cstdint>

#define VERBOSE 1

#define PLANE_PLACES 10
#define PLANE_MIN_TIME 10 // in sec
#define PLANE_MAX_TIME 100 // in sec
#define PLANE_MAX_ALLOWED_BAGGAGE_WEIGHT 75 // kg

#define SECURITY_SELECTOR_DELAY 10 // in ms
#define SECURITY_GATE_MAX_DELAY 50 // in ms
#define SECURITY_GATE_MIN_DELAY 20 // in ms

#define STAIRS_MAX_ALLOWED_OCCUPANCY PLANE_PLACES / 2

#define SEM_INIT_VALUE 0
#define SEM_NUMBER 24

enum Colors
{
        NONE = 0,
        RED,
        GREEN,
        YELLOW,
        BLUE,
        MAGENTA,
        CYAN,
};

const std::string colors[] = {
        "\033[0m",
        "\033[31m",
        "\033[32m",
        "\033[33m",
        "\033[34m",
        "\033[35m",
        "\033[36m",
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
        FIFO_BAGGAGE_CONTROL = 0,
        FIFO_SECURITY_CONTROL,
        FIFO_SECURITY_SELECTOR,
        FIFO_SECURITY_GATE_0,
        FIFO_SECURITY_GATE_1,
        FIFO_SECURITY_GATE_2,
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
        SEM_BAGGAGE_CONTROL_ENTRANCE = 0,
        SEM_BAGGAGE_CONTROL_OUT,
        SEM_SECURITY_CONTROL_ENTRANCE,
        SEM_SECURITY_CONTROL_ENTRANCE_WAIT,
        SEM_SECURITY_CONTROL_SELECTOR_ENTRANCE_WAIT,
        SEM_SECURITY_CONTROL_SELECTOR_WAIT,
        SEM_SECURITY_GATE_0,
        SEM_SECURITY_GATE_1,
        SEM_SECURITY_GATE_2,
        SEM_SECURITY_GATE_0_WAIT,
        SEM_SECURITY_GATE_1_WAIT,
        SEM_SECURITY_GATE_2_WAIT,
        SEM_SECURITY_CONTROL_OUT,
        SEM_STAIRS_PASSENGER_IN,
        SEM_STAIRS_PASSENGER_WAIT,
        SEM_PLANE_PASSANGER_IN,
        SEM_PLANE_PASSANGER_WAIT,
        SEM_STAIRS_COUNTER,
        SEM_PLANE_COUNTER,
        SEM_PASSENGER_COUNTER,
        SEM_PLANE_WAIT,
        SEM_STAIRS_WAIT,
        SEM_PLANE_DEPART,
        SEM_SECURITY_CONTROL_SELECTOR,
};

enum Signals
{
        SIGNAL_PASSENGER_IS_OVERWEIGHT = 35,
        SIGNAL_PASSENGER_IS_DANGEROUS,
        SIGNAL_PASSENGER_SKIPPED,
        SIGNAL_STAIRS_CLOSE,
        SIGNAL_PLANE_GO,
        SIGNAL_PLANE_READY,
        SIGNAL_PLANE_READY_DEPART,
        SIGNAL_DISPATCHER_PLANE_FORCED_DEPART,
        SIGNAL_OK,
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

enum EventSignals
{
        TRIGGER_AGRESSIVE = SIGNAL_OK + 1, // INFO: signals event handler that passenger is aggressive [passenger -> eventHandler]
        TRIGGER_DANGEROUS, // INFO: signals event handler that passenger has dangerous baggage [secControl -> eventHandler]
        TRIGGER_OVERWEIGHT, // INFO: signals event handler that passenger is overweight [baggageControl -> eventHandler]
};

std::vector<int> initSemaphores(int permissions, size_t numPassengers);

void genRandomVector(std::vector<uint64_t> &vec, uint64_t min, uint64_t max);
void genRandom(uint64_t &val, uint64_t min, uint64_t max);

void vCout(const std::string &msg, int color = 0, int log = 0);

void createSubprocesses(size_t n, std::vector<pid_t> &pids, const std::vector<std::string> &names);
void createSubprocess(pid_t &pid, const std::string &name);

void safeFIFOOpen(int &fd, const std::string &name, int flags);
void safeFIFOWrite(int fd, const void *buf, size_t size);
void safeFIFORead(int fd, void *buf, size_t size);
void safeFIFOClose(int fd);

void safeSemop(int semID, struct sembuf *sops, size_t nsops);
int safeGetSemVal(int semID, int semnum);
void safeSetSemVal(int semID, int semnum, int val);

#endif // UTILS_H
