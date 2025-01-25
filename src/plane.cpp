#include <iostream>
#include <cstdint>
#include <random>
#include <climits>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <mutex>
#include <sys/sem.h>

#include "utils.h"
#include "plane.h"

extern sembuf INC_SEM;
extern sembuf DEC_SEM;

static bool leaveEarly = false;

void planeSignalHandler(int signum)
{
        switch (signum)
        {
        case SIGNAL_OK:
                vCout("Plane: Received signal OK\n", CYAN, LOG_PLANE);
                break;
        case SIGNAL_PLANE_GO:
                vCout("Plane: Received signal PLANE_GO\n", CYAN, LOG_PLANE);
                leaveEarly = true;
                break;
        case SIGTERM:
                vCout("Plane: Received signal SIGTERM\n", CYAN, LOG_PLANE);
                exit(0);
                break;
        default:
                vCout("Plane: Received unknown signal\n", CYAN, LOG_PLANE);
                break;
        }
}

void planeProcess(PlaneProcessArgs args)
{
        vCout("Plane process: " + std::to_string(args.id) + "\n", CYAN, LOG_PLANE);

        Plane plane(args.id);

        // attach handler to signals
        struct sigaction sa;
        sa.sa_handler = planeSignalHandler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;
        if (sigaction(SIGNAL_OK, &sa, NULL) == -1)
        {
                perror("sigaction");
                exit(1);
        }
        if (sigaction(SIGNAL_PLANE_GO, &sa, NULL) == -1)
        {
                perror("sigaction");
                exit(1);
        }
        if (sigaction(SIGTERM, &sa, NULL) == -1)
        {
                perror("sigaction");
                exit(1);
        }

        sigval sig;
        sig.sival_int = args.pid;

        while (true)
        {
                sigqueue(args.pidDispatcher, SIGNAL_PLANE_READY, sig);
                leaveEarly = false;
                safeSemop(args.semIDPlaneWait, &DEC_SEM, 1);
                while (true)
                {
                        vCout("Plane " + std::to_string(args.id) + ": waiting for passenger\n");
                        safeSemop(args.semIDPlanePassengerIn, &DEC_SEM, 1);
                        safeSemop(args.semIDPlanePassengerWait, &INC_SEM, 1);

                        // occupancy --

                        safeSemop(args.semIDStairsCounter, &INC_SEM, 1);

                        safeSemop(args.semIDPlaneCounter, &INC_SEM, 1);

                        int passengersOnBoard = safeGetSemVal(args.semIDPlaneCounter, 0);
                        if (passengersOnBoard == plane.getMaxPassengers())
                        {
                                vCout("Plane: Plane is full\n");
                                // set counter to 0
                                safeSetSemVal(args.semIDPlaneCounter, 0, 0);
                                safeSemop(args.semIDStairsCounter, &INC_SEM, 1);
                                safeSetSemVal(args.semIDStairsCounter, 0, STAIRS_MAX_ALLOWED_OCCUPANCY);
                                sigqueue(args.pidDispatcher, SIGNAL_PLANE_READY_DEPART, sig);
                                safeSemop(args.semIDPlaneDepart, &DEC_SEM, 1);
                                break;
                        }
                        if (leaveEarly && safeGetSemVal(args.semIDStairsCounter, 0) == STAIRS_MAX_ALLOWED_OCCUPANCY)
                        {
                                vCout("Plane " + std::to_string(args.id) + ": leaving early\n");
                                safeSemop(args.semIDStairsCounter, &INC_SEM, 1);
                                safeSetSemVal(args.semIDStairsCounter, 0, STAIRS_MAX_ALLOWED_OCCUPANCY);
                                sigqueue(args.pidDispatcher, SIGNAL_PLANE_READY_DEPART, sig);
                                safeSemop(args.semIDPlaneDepart, &DEC_SEM, 1);
                                break;
                        }
                }
                vCout("Plane " + std::to_string(args.id) + ": leaving for " + std::to_string(plane.getTimeOfCycle()) + " sec\n", CYAN, LOG_PLANE);
                usleep(plane.getTimeOfCycle() * 1000);
        }
}

std::vector<pid_t> initPlanes(size_t num, PlaneProcessArgs args)
{
        std::vector<pid_t> planePIDs;
        pid_t oldPID = getpid();
        vCout("Init planes\n", NONE, LOG_MAIN);
        for (size_t i = 0; i < num; i++)
        {
                pid_t newPID;
                createSubprocess(newPID, "plane");
                planePIDs.push_back(newPID);
                if (getpid() != oldPID)
                {
                        args.id = i;
                        args.pid = getpid();
                        planeProcess(args);
                        exit(0);
                }
        }

        vCout("All planes created\n", NONE, LOG_MAIN);

        return planePIDs;
}

Plane::Plane(uint64_t id)
        : mID(id), mMaxPassengers(PLANE_PLACES), mMaxBaggageWeight(PLANE_MAX_ALLOWED_BAGGAGE_WEIGHT)
{
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<uint64_t> disTime(PLANE_MIN_TIME, PLANE_MAX_TIME);
        mTimeOfCycle = disTime(gen);

        vCout("Plane " + std::to_string(mID) + " created with the following properties:\n", CYAN, LOG_PLANE);
        vCout("Max passengers: " + std::to_string(mMaxPassengers) + "\n", CYAN, LOG_PLANE);
        vCout("Max baggage weight: " + std::to_string(mMaxBaggageWeight) + "\n", CYAN, LOG_PLANE);
        vCout("Time of cycle: " + std::to_string(mTimeOfCycle) + "\n", CYAN, LOG_PLANE);
}

Plane::Plane(uint64_t id, uint64_t maxPassengers, uint64_t maxBaggageWeight, uint64_t timeOfCycle)
        : mID(id), mMaxPassengers(maxPassengers), mMaxBaggageWeight(maxBaggageWeight), mTimeOfCycle(timeOfCycle)
{
        vCout("Plane " + std::to_string(mID) + " created with the following properties:\n", CYAN, LOG_PLANE);
        vCout("Max passengers: " + std::to_string(mMaxPassengers) + "\n", CYAN, LOG_PLANE);
        vCout("Max baggage weight: " + std::to_string(mMaxBaggageWeight) + "\n", CYAN, LOG_PLANE);
        vCout("Time of cycle: " + std::to_string(mTimeOfCycle) + "\n", CYAN, LOG_PLANE);
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
