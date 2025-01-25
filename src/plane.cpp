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

void planeSignalHandler(int signum)
{
        switch (signum)
        {
        case SIGNAL_OK:
                vCout("Plane: Received signal OK\n");
                break;
        case SIGNAL_PLANE_GO:
                vCout("Plane: Received signal PLANE_GO\n");
                break;
        case SIGTERM:
                vCout("Plane: Received signal SIGTERM\n");
                exit(0);
                break;
        default:
                vCout("Plane: Received unknown signal\n");
                break;
        }
}

void planeProcess(PlaneProcessArgs args)
{
        vCout("Plane process: " + std::to_string(args.id) + "\n");

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

        // TODO: run plane tasks here

        sigval sig;
        sig.sival_int = args.pid;

        while (true)
        {
                sigqueue(args.pidDispatcher, SIGNAL_PLANE_READY, sig);

                while(semop(args.semIDPlaneWait, &DEC_SEM, 1) == -1)
                {
                        if (errno == EINTR)
                        {
                                continue;
                        }
                        perror("semop");
                        exit(1);
                }

                while (true)
                {
                        vCout("Plane " + std::to_string(args.id) + ": waiting for passenger\n");
                        while (semop(args.semIDPlanePassengerIn, &DEC_SEM, 1) == -1)
                        {
                                if (errno == EINTR)
                                {
                                        continue;
                                }
                                perror("semop");
                                exit(1);
                        }
                        while (semop(args.semIDPlanePassengerWait, &INC_SEM, 1) == -1)
                        {
                                if (errno == EINTR)
                                {
                                        continue;
                                }
                                perror("semop");
                                exit(1);
                        }

                        // occupancy --

                        while (semop(args.semIDStairsCounter, &DEC_SEM, 1) == -1)
                        {
                                if (errno == EINTR)
                                {
                                        continue;
                                }
                                perror("semop");
                                exit(1);
                        }

                        while (semop(args.semIDPlaneCounter, &INC_SEM, 1) == -1)
                        {
                                if (errno == EINTR)
                                {
                                        continue;
                                }
                                perror("semop");
                                exit(1);
                        }

                        int passengersOnBoard;
                        while ((passengersOnBoard = semctl(args.semIDPlaneCounter, 0, GETVAL)) == -1)
                        {
                                if (errno == EINTR)
                                {
                                        continue;
                                }
                                perror("semctl");
                                exit(1);
                        }
                        if (passengersOnBoard == plane.getMaxPassengers())
                        {
                                vCout("Plane: Plane is full\n");
                                // set counter to 0
                                while (semctl(args.semIDPlaneCounter, 0, SETVAL, 0) == -1)
                                {
                                        if (errno == EINTR)
                                        {
                                                continue;
                                        }
                                        perror("semctl");
                                        exit(1);
                                }
                                while (semctl(args.semIDStairsCounter, 0, SETVAL, 0) == -1)
                                {
                                        if (errno == EINTR)
                                        {
                                                continue;
                                        }
                                        perror("semctl");
                                        exit(1);
                                }
                                sigqueue(args.pidDispatcher, SIGNAL_PLANE_READY_DEPART, sig);

                                pause(); // wait for signal to go
                                break;
                        }
                }
                vCout("Plane " + std::to_string(args.id) + ": leaving for " + std::to_string(plane.getTimeOfCycle()) + " sec\n");
                usleep(plane.getTimeOfCycle() * 1000);
        }
}

void initPlanes(size_t num, PlaneProcessArgs args)
{
        pid_t oldPID = getpid();
        vCout("Init planes\n");
        for (size_t i = 0; i < num; i++)
        {
                pid_t newPID;
                createSubprocess(newPID, "plane");
                if (getpid() != oldPID)
                {
                        args.id = i;
                        args.pid = getpid();
                        planeProcess(args);
                        exit(0);
                }
        }

        vCout("All planes created\n");
}

Plane::Plane(uint64_t id)
        : mID(id), mMaxPassengers(PLANE_PLACES), mMaxBaggageWeight(PLANE_MAX_ALLOWED_BAGGAGE_WEIGHT)
{
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<uint64_t> disTime(PLANE_MIN_TIME, PLANE_MAX_TIME);
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
