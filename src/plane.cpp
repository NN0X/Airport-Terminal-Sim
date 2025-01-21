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

std::mutex planeStairsMutex;
int passengersOnBoard = 0;

extern int stairsOccupancy;

void planeSignalHandler(int signum)
{
        switch (signum)
        {
        case SIGNAL_OK:
                syncedCout("Plane: Received signal OK\n");
                break;
        case SIGNAL_PLANE_RECEIVE:
                syncedCout("Plane: Received signal PLANE_RECEIVE\n");
                break;
        case SIGNAL_PLANE_GO:
                syncedCout("Plane: Received signal PLANE_GO\n");
                break;
        case SIGTERM:
                syncedCout("Plane: Received signal SIGTERM\n");
                exit(0);
                break;
        default:
                syncedCout("Plane: Received unknown signal\n");
                break;
        }
}

void planeProcess(size_t id, int semIDPlaneStairs1, int semIDPlaneStairs2, pid_t stairsPid, pid_t dispatcherPid)
{
        syncedCout("Plane process: " + std::to_string(id) + "\n");

        Plane plane(id);

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
        if (sigaction(SIGNAL_PLANE_RECEIVE, &sa, NULL) == -1)
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

        // 1. wait for signal PLANE_RECEIVE
        // 2. let passengers board (semaphore)
        // 3. increment passengersOnBoard
        // 4. decrement occupancy shared with stairs and signal stairs PASSENGER_LEFT_STAIRS
        // 5. if passengersOnBoard == plane.getMaxPassengers() signal dispatcher PLANE_READY
        //    5.1. wait for signal PLANE_GO
        // 6. if at any point signal PLANE_GO is received when passengersOnBoard < plane.getMaxPassengers() continue receiving passengers until occupancy is 0
        // 7. simulate delay
        // 8. repeat until signal SIGTERM

        sigval sig;
        sig.sival_int = getpid();

        while (true)
        {
                sigqueue(dispatcherPid, SIGNAL_PLANE_READY, sig);
                pause(); // wait for signal PLANE_RECEIVE
                while (true)
                {
                        syncedCout("Plane " + std::to_string(getpid()) + ": waiting for passenger\n");
                        sembuf dec = {0, -1, 0};
                        while (semop(semIDPlaneStairs1, &dec, 1) == -1)
                        {
                                if (errno == EINTR)
                                {
                                        continue;
                                }
                                perror("semop");
                                exit(1);
                        }
                        sembuf inc = {0, 1, 0};
                        while (semop(semIDPlaneStairs2, &inc, 1) == -1)
                        {
                                if (errno == EINTR)
                                {
                                        continue;
                                }
                                perror("semop");
                                exit(1);
                        }
                        planeStairsMutex.lock();
                        passengersOnBoard++;
                        stairsOccupancy--;
                        kill(stairsPid, SIGNAL_PASSENGER_LEFT_STAIRS);
                        if (passengersOnBoard == plane.getMaxPassengers())
                        {
                                syncedCout("Plane: Plane is full\n");
                                sigqueue(dispatcherPid, SIGNAL_PLANE_READY_DEPART, sig);
                                pause(); // wait for signal PLANE_GO
                                break;
                        }
                        planeStairsMutex.unlock();
                }
                syncedCout("Plane " + std::to_string(getpid()) + ": leaving for " + std::to_string(plane.getTimeOfCycle()) + " sec\n");
                usleep(plane.getTimeOfCycle() * 1000);
        }
}

std::vector<pid_t> initPlanes(size_t num, int semIDPlaneStairs1, int semIDPlaneStairs2, pid_t stairsPid, pid_t dispatcherPid)
{
        pid_t pid = getpid();
        std::vector<pid_t> pids(1);
        pids[0] = pid;
        syncedCout("Init planes\n");
        for (size_t i = 0; i < num; i++)
        {
                createSubprocesses(1, pids, {"plane"});
                if (getpid() != pid)
                {
                        planeProcess(i, semIDPlaneStairs1, semIDPlaneStairs2, stairsPid, dispatcherPid);
                        exit(0);
                }
        }

        syncedCout("All planes created\n");

        return std::vector<pid_t>(pids.begin() + 1, pids.end());
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
