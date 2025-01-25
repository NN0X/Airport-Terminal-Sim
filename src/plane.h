#ifndef PLANE_H
#define PLANE_H

#include <cstdint>
#include <unistd.h>
#include <vector>
#include <sys/types.h>

#include "args.h"

void planeProcess(PlaneProcessArgs args);
std::vector<pid_t> initPlanes(size_t num, PlaneProcessArgs args);

class Plane
{
private:
        uint64_t mID;
        uint64_t mMaxPassengers;
        uint64_t mMaxBaggageWeight; // kg
        uint64_t mTimeOfCycle; // sec

public:
        Plane(uint64_t id);
        Plane(uint64_t id, uint64_t maxPassengers, uint64_t maxBaggageWeight, uint64_t timeToComeback);
        uint64_t getID() const;
        uint64_t getMaxPassengers() const;
        uint64_t getMaxBaggageWeight() const;
        uint64_t getTimeOfCycle() const;
};

#endif // PLANE_H
