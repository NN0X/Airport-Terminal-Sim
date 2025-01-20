#ifndef UTILS_H
#define UTILS_H

#include <vector>
#include <string>
#include <sys/types.h>
#include <signal.h>
#include <cstdint>

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
        MAIN,
        BAGGAGE_CONTROL,
        SEC_CONTROL,
        DISPATCHER,
        STAIRS,
};

enum FIFOs
{
        BAGGAGE_CONTROL_FIFO = 0,
        SEC_CONTROL_FIFO,
        SEC_SELECTOR_FIFO,
        SEC_GATE_0_FIFO,
        SEC_GATE_1_FIFO,
        SEC_GATE_2_FIFO,
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
        BAGGAGE_CTRL = 0, // INFO: 
        SEC_CTRL, // INFO: 
        SEC_GATE_0, // INFO: 
        SEC_GATE_1, // INFO: 
        SEC_GATE_2, // INFO: 
};

enum Signals
{
        PASSENGER_IS_OVERWEIGHT = 35, // INFO: signals baggage control that passenger is overweight [baggageControl -> passenger]
        PASSENGER_IS_DANGEROUS, // INFO: signals passenger that he has dangerous baggage [secControl -> passenger]
        SIGNAL_OK, // INFO: signals that everything is ok [any -> any]
};

enum EventSignals
{
        TRIGGER_AGRESSIVE = SIGNAL_OK + 1, // INFO: signals event handler that passenger is aggressive [passenger -> eventHandler]
        TRIGGER_DANGEROUS, // INFO: signals event handler that passenger has dangerous baggage [secControl -> eventHandler]
        TRIGGER_OVERWEIGHT, // INFO: signals event handler that passenger is overweight [baggageControl -> eventHandler]

        EVENT_KILL, // INFO: signals passenger process to exit [eventHandler -> passenger]
};

std::vector<int> initSemaphores(int permissions);
void genRandomVector(std::vector<uint64_t> &vec, uint64_t min, uint64_t max);
void vCout(const std::string &msg, int color = 0);
void syncedCout(const std::string &msg, int color = 0);
void createSubprocesses(size_t n, std::vector<pid_t> &pids, const std::vector<std::string> &names);

#endif // UTILS_H
