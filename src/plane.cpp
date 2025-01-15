#include <iostream>
#include <cstdint>
#include <random>
#include <climits>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

#include "utils.h"
#include "plane.h"

void planeProcess(size_t id, int semIDPlaneStairs)
{
        syncedCout("Plane process: " + std::to_string(id) + "\n");

        Plane plane(id);

        // TODO: run plane tasks here

        // INFO: plane waits for terminal to be free
        // INFO: plane waits for passengers to board
        // INFO: plane waits x time
        // INFO: repeat until signal to exit
}

void initPlanes(size_t num, int semIDPlaneStairs)
{
        pid_t pid = getpid();
        for (size_t i = 0; i < num; i++)
        {
                std::vector<pid_t> pids(1);
                pids[0] = pid;
                createSubprocesses(1, pids, {"plane"});
                if (getpid() != pids[0])
                {
                        planeProcess(i, semIDPlaneStairs);
                        exit(0);
                }
        }

        syncedCout("All planes created\n");
}

Plane::Plane(uint64_t id)
        : mID(id), mMaxPassengers(PLANE_PLACES), mMaxBaggageWeight(MAX_ALLOWED_BAGGAGE_WEIGHT)
{
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<uint64_t> disTime(MIN_TIME, MAX_TIME);
        mTimeOfCycle = disTime(gen);

        vCout("Plane " + std::to_string(mID) + " created with the following properties:\n");
        vCout("Max passengers: " + std::to_string(mMaxPassengers) + "\n");
        vCout("Max baggage weight: " + std::to_string(mMaxBaggageWeight) + "\n");
        vCout("Time of cycle: " + std::to_string(mTimeOfCycle) + "\n");
}

Plane::Plane(uint64_t id, uint64_t maxPassengers, uint64_t maxBaggageWeight, uint64_t timeOfCycle)
        : mID(id), mMaxPassengers(maxPassengers), mMaxBaggageWeight(maxBaggageWeight), mTimeOfCycle(timeOfCycle)
{
        vCout("Plane " + std::to_string(mID) + " created with the following properties:\n");
        vCout("Max passengers: " + std::to_string(mMaxPassengers) + "\n");
        vCout("Max baggage weight: " + std::to_string(mMaxBaggageWeight) + "\n");
        vCout("Time of cycle: " + std::to_string(mTimeOfCycle) + "\n");
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
