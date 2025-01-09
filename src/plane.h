#ifndef PLANE_H
#define PLANE_H

#include <cstdint>

class Plane
{
private:
	uint64_t id;
	uint64_t maxPassengers;
	uint64_t maxBaggageWeight; // in arbitrary units
	uint64_t timeToComeback; // in arbitrary units

public:
	Plane(uint64_t id, uint64_t maxPassengers, uint64_t maxBaggageWeight, uint64_t timeToComeback);
	uint64_t getId() const;
	uint64_t getMaxPassengers() const;
	uint64_t getMaxBaggageWeight() const;
	uint64_t getTimeToComeback() const;
};

#endif // PLANE_H
