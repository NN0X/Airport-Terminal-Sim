#ifndef PASSENGER_H
#define PASSENGER_H

#define VIP_PROBABILITY 0.1
#define MAX_BAGGAGE_WEIGHT 100
#define DANGEROUS_BAGGAGE_PROBABILITY 0.05
#define MAX_PASSENGER_DELAY 100 // in ms

#include <cstdint>
#include <sys/types.h>
#include <vector>

#include "plane.h"
#include "args.h"

/*
* main function of passenger process
*/
void passengerProcess(PassengerProcessArgs args);

/*
* main function of spawnPassengers process
*/
void spawnPassengers(size_t num, const std::vector<uint64_t> &delays, PassengerProcessArgs args);

/*
* Structure for fifo between baggage control and passenger
*/
struct BaggageInfo
{
        pid_t pid;
        uint64_t weight;
};


/*
* Structure for fifo between security control and passenger
*/
struct TypeInfo
{
        int id;
        pid_t pid;
        bool type;
        bool isVIP;
};

/*
* Structure for fifo between security gate threads and passenger
*/
struct DangerInfo
{
        pid_t pid;
        bool hasDangerousBaggage;
};

/*
* Structure for fifo between security selector thread and passenger
*/
struct SelectedPair
{
        int passengerIndex;
        int gateIndex;
};

/*
* class for passenger
*/
class Passenger
{
public:
        uint64_t mID;
        bool mIsVip;
        bool mType; // each gate can have 2 same type passengers at the same time
        uint64_t mBaggageWeight; // kg
        uint64_t mPersonalBaggageWeight; // kg
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
