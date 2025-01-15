#ifndef UTILS_H
#define UTILS_H

#include <vector>
#include <string>
#include <sys/types.h>

enum ProcessTypes
{
        MAIN,
        BAGGAGE_CONTROL,
        SEC_CONTROL,
        DISPATCHER,
        STAIRS,
};

enum Semaphores
{
        BAGGAGE_CTRL = 0, // INFO: tells passenger to enter baggage control [baggageControl -> passenger]
        SEC_GATE, // INFO: tells passenger to enter security control [secControl -> passenger]
        GATE, // INFO: tells passengers enter stairs [stairs -> passenger]
        PLANE_STAIRS, // INFO: tells plane to wait for passengers [stairs -> plane]
};

enum Signals
{
        BAGGAGE_CTRL_OPEN = 0, // INFO: opens baggage control [dispatcher -> baggageControl]
        BAGGAGE_CTRL_CLOSE, // INFO: closes baggage control [dispatcher -> baggageControl]
        SEC_GATE_OPEN, // INFO: opens security gate [dispatcher -> secControl]
        SEC_GATE_CLOSE, // INFO: closes security gate [dispatcher -> secControl]
        PLANE_GATE_OPEN, // INFO: opens plane gate [dispatcher -> stairs]
        PLANE_GATE_CLOSE, // INFO: closes plane gate [dispatcher -> stairs]
        PLANE_READY, // INFO: plane is ready to board passengers [plane -> dispatcher]
        PLANE_DEPART, // INFO: signals plane to depart [dispatcher -> plane]
        PLANE_FULL, // INFO: signals plane that it is full [plane -> dispatcher]
        PASSENGER_IS_VIP_SEC, // INFO: signals security control that passenger is VIP [passenger -> secControl]
        PASSENGER_IS_OVERWEIGHT, // INFO: signals baggage control that passenger is overweight [baggageControl -> passenger]
        PASSENGER_IS_DANGEROUS, // INFO: signals passenger that he has dangerous baggage [baggageControl -> passenger]
};

enum EventSignals
{
        TRIGGER_AGRESSIVE = 0, // INFO: signals event handler that passenger is aggressive [passenger -> eventHandler]
        TRIGGER_DANGEROUS, // INFO: signals event handler that passenger has dangerous baggage [secControl -> eventHandler]
        TRIGGER_OVERWEIGHT, // INFO: signals event handler that passenger is overweight [baggageControl -> eventHandler]

        EVENT_KILL, // INFO: signals passenger process to exit [eventHandler -> passenger]
};

std::vector<int> initSemaphores(int permissions);
void genRandomVector(std::vector<uint64_t> &vec, uint64_t min, uint64_t max);
void vCout(const std::string &msg);
void syncedCout(const std::string &msg);
void createSubprocesses(size_t n, std::vector<pid_t> &pids, const std::vector<std::string> &names);

#endif // UTILS_H
