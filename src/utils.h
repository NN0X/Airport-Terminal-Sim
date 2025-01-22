#ifndef UTILS_H
#define UTILS_H

#include <vector>
#include <string>
#include <sys/types.h>
#include <signal.h>
#include <cstdint>

#define VERBOSE 0

#define PLANE_PLACES 10
#define PLANE_MIN_TIME 10 // in sec
#define PLANE_MAX_TIME 1000 // in sec
#define PLANE_MAX_ALLOWED_BAGGAGE_WEIGHT 75 // kg

#define SECURITY_SELECTOR_DELAY 100 // in ms
#define SECURITY_GATE_MAX_DELAY 500 // in ms
#define SECURITY_GATE_MIN_DELAY 200 // in ms

#define STAIRS_MAX_ALLOWED_OCCUPANCY PLANE_PLACES / 2

#define SEM_INIT_VALUE 0
#define SEM_NUMBER 18

enum Colors
{
        COLOR_NONE = 0,
        COLOR_RED,
        COLOR_GREEN,
        COLOR_YELLOW,
        COLOR_BLUE,
        COLOR_MAGENTA,
        COLOR_CYAN,
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
        SEM_SECURITY_GATE_0,
        SEM_SECURITY_GATE_1,
        SEM_SECURITY_GATE_2,
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
        SEM_SECURITY_CONTROL_SELECTOR,
};

enum Signals
{
        SIGNAL_PASSENGER_IS_OVERWEIGHT = 35, // INFO: signals baggage control that passenger is overweight [baggageControl -> passenger]
        SIGNAL_PASSENGER_IS_DANGEROUS, // INFO: signals passenger that he has dangerous baggage [secControl -> passenger]
        SIGNAL_PASSENGER_LEFT_STAIRS, // INFO: signals stairs that passenger left stairs [stairs -> stairs]
        SIGNAL_STAIRS_OPEN, // INFO: signals stairs that stairs are open [dispatcher -> stairs]
        SIGNAL_STAIRS_CLOSE, // INFO: signals stairs that stairs are closed [dispatcher -> stairs]
        SIGNAL_PLANE_RECEIVE, // INFO: signals plane that it can receive passengers [dispatcher -> plane]
        SIGNAL_PLANE_GO, // INFO: signals plane that it can go to terminal [dispatcher -> plane]
        SIGNAL_PLANE_READY, // INFO: signals dispatcher that plane is ready to receive passengers [plane -> dispatcher]
        SIGNAL_PLANE_READY_DEPART, // INFO: signals plane that it can depart [plane -> dispatcher]
        SIGNAL_OK, // INFO: signals that everything is ok [any -> any]
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

void vCout(const std::string &msg, int color = 0);

void createSubprocesses(size_t n, std::vector<pid_t> &pids, const std::vector<std::string> &names);
void createSubprocess(pid_t &pid, const std::string &name);

#endif // UTILS_H
