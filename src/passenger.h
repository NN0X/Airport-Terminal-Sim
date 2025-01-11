#ifndef PASSENGER_H
#define PASSENGER_H

#include <cstdint>

#include "plane.h"

class Passenger
{
public: // temporary
	uint64_t id;
	bool isVip;
	bool type; // each gate can have 2 same type passengers at the same time
	uint64_t baggageWeight; // in arbitrary units
	bool isAggressive; // causes random negative events
        bool hasDangerousBaggage; // causes event at security control

public:
        Passenger();
	Passenger(uint64_t id, bool isVip, bool type, uint64_t baggageWeight);
	uint64_t getId() const;
	bool getIsVip() const;
	bool getType() const;
	uint64_t getBaggageWeight() const;
};

inline bool isSameType(const Passenger& p1, const Passenger& p2)
{
	return p1.getType() == p2.getType();
}

inline bool isBaggageOverweight(const Passenger& p, const Plane& plane)
{
	return p.getBaggageWeight() > plane.getMaxBaggageWeight();
}

#endif // PASSENGER_H
