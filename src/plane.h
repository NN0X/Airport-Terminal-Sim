#ifndef PLANE_H
#define PLANE_H

#include <cstdint>

#define PLANE_PLACES 100
#define MIN_TIME 10
#define MAX_TIME 1000
#define MAX_ALLOWED_BAGGAGE_WEIGHT 75

class Plane
{
private:
        uint64_t mID;
        uint64_t mMaxPassengers;
        uint64_t mMaxBaggageWeight; // in arbitrary units
        uint64_t mTimeOfCycle; // in arbitrary units

public:
        Plane(uint64_t id);
        Plane(uint64_t id, uint64_t maxPassengers, uint64_t maxBaggageWeight, uint64_t timeToComeback);
        uint64_t getID() const;
        uint64_t getMaxPassengers() const;
        uint64_t getMaxBaggageWeight() const;
        uint64_t getTimeOfCycle() const;
};

#endif // PLANE_H
