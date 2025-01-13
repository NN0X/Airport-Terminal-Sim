#ifndef PASSENGER_H
#define PASSENGER_H

#define VIP_PROBABILITY 0.1
#define MAX_BAGGAGE_WEIGHT 100
#define DANGEROUS_BAGGAGE_PROBABILITY 0.05

#include <cstdint>

#include "plane.h"

class Passenger
{
public: // temporary
        uint64_t mID;
        bool mIsVip;
        bool mType; // each gate can have 2 same type passengers at the same time
        uint64_t mBaggageWeight; // in arbitrary units
        bool mIsAggressive; // causes random negative events
        bool mHasDangerousBaggage; // causes event at security control

public:
        Passenger(uint64_t id);
        Passenger(uint64_t id, bool isVip, bool type, uint64_t baggageWeight, bool hasDangerousBaggage);
        uint64_t getID() const;
        bool getIsVip() const;
        bool getType() const;
        uint64_t getBaggageWeight() const;
        bool getIsAggressive() const;
        bool getHasDangerousBaggage() const;
};

namespace PaUtils
{
        inline bool isSameType(const Passenger& p1, const Passenger& p2)
        {
                return p1.getType() == p2.getType();
        }

        inline bool isBaggageOverweight(const Passenger& p, const Plane& plane)
        {
                return p.getBaggageWeight() > plane.getMaxBaggageWeight();
        }
};

#endif // PASSENGER_H
