#include <iostream>
#include <cstdint>
#include <random>
#include <climits>

#include "plane.h"

Plane::Plane(uint64_t id)
        : mID(id), mMaxPassengers(PLANE_PLACES), mMaxBaggageWeight(MAX_ALLOWED_BAGGAGE_WEIGHT)
{
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<uint64_t> disTime(MIN_TIME, MAX_TIME);
        mTimeOfCycle = disTime(gen);

        std::cout << "Plane " << mID << " created with the following properties:\n";
        std::cout << "Max passengers: " << mMaxPassengers << "\n";
        std::cout << "Max baggage weight: " << mMaxBaggageWeight << "\n";
        std::cout << "Time of cycle: " << mTimeOfCycle << "\n";
}

Plane::Plane(uint64_t id, uint64_t maxPassengers, uint64_t maxBaggageWeight, uint64_t timeOfCycle)
        : mID(id), mMaxPassengers(maxPassengers), mMaxBaggageWeight(maxBaggageWeight), mTimeOfCycle(timeOfCycle)
{
        std::cout << "Plane " << mID << " created with the following properties:\n";
        std::cout << "Max passengers: " << mMaxPassengers << "\n";
        std::cout << "Max baggage weight: " << mMaxBaggageWeight << "\n";
        std::cout << "Time to comeback: " << mTimeOfCycle << "\n";
}

uint64_t Plane::getID() const
{
        return mID;
}

uint64_t Plane::getMaxPassengers() const
{
        return mMaxPassengers;
}

uint64_t Plane::getMaxBaggageWeight() const
{
        return mMaxBaggageWeight;
}

uint64_t Plane::getTimeOfCycle() const
{
        return mTimeOfCycle;
}
