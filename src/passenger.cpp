#include <iostream>
#include <random>
#include <cstdint>
#include <climits>

#include "passenger.h"

Passenger::Passenger(uint64_t id)
        : mID(id), mIsAggressive(false)
{
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<uint64_t> dis(0, UINT64_MAX);

        // vip == true when random number < VIP_PROBABILITY * UINT64_MAX
        mIsVip = dis(gen) < VIP_PROBABILITY * UINT64_MAX;
        mType = dis(gen) < 0.5 * UINT64_MAX;
        mBaggageWeight = dis(gen) % MAX_BAGGAGE_WEIGHT;
        mHasDangerousBaggage = dis(gen) < DANGEROUS_BAGGAGE_PROBABILITY * UINT64_MAX;


        std::cout << "Passenger " << mID << " created with the following properties:\n";
        std::cout << "VIP: " << mIsVip << "\n";
        std::cout << "Type: " << mType << "\n";
        std::cout << "Baggage weight: " << mBaggageWeight << "\n";
        std::cout << "Aggressive: " << mIsAggressive << "\n";
        std::cout << "Dangerous baggage: " << mHasDangerousBaggage << "\n";
}

Passenger::Passenger(uint64_t id, bool isVip, bool type, uint64_t baggageWeight, bool hasDangerousBaggage)
        : mID(id), mIsVip(isVip), mType(type), mBaggageWeight(baggageWeight), mIsAggressive(false), mHasDangerousBaggage(hasDangerousBaggage)
{
        std::cout << "Passenger " << mID << " created with the following properties:\n";
        std::cout << "VIP: " << mIsVip << "\n";
        std::cout << "Type: " << mType << "\n";
        std::cout << "Baggage weight: " << mBaggageWeight << "\n";
        std::cout << "Aggressive: " << mIsAggressive << "\n";
        std::cout << "Dangerous baggage: " << mHasDangerousBaggage << "\n";
}

uint64_t Passenger::getID() const
{
        return mID;
}

bool Passenger::getIsVip() const
{
        return mIsVip;
}

bool Passenger::getType() const
{
        return mType;
}

uint64_t Passenger::getBaggageWeight() const
{
        return mBaggageWeight;
}

bool Passenger::getIsAggressive() const
{
        return mIsAggressive;
}

bool Passenger::getHasDangerousBaggage() const
{
        return mHasDangerousBaggage;
}
