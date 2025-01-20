#ifndef PLANE_H
#define PLANE_H

#include <cstdint>
#include <unistd.h>
#include <sys/types.h>

#define PLANE_PLACES 100
#define MIN_TIME 10 // in sec
#define MAX_TIME 1000 // in sec
#define MAX_ALLOWED_BAGGAGE_WEIGHT 75 // kg

void planeProcess(size_t id, int semIDPlaneStairs, pid_t stairsPid, pid_t dispatcherPid);
void initPlanes(size_t num, int semIDPlaneStairs, pid_t stairsPid, pid_t dispatcherPid);

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
