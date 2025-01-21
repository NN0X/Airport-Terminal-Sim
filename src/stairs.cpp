#include <iostream>
#include <cstdint>
#include <sys/types.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <errno.h>
#include <mutex>

#include "plane.h"
#include "utils.h"

#define MAX_ALLOWED_OCCUPANCY PLANE_PLACES / 2

int stairsOccupancy = 0;
static bool stairsOpen = false;

extern std::mutex planeStairsMutex;
extern int passengersOnBoard;

void stairsSignalHandler(int signum)
{
        switch (signum)
        {
        case SIGNAL_OK:
                syncedCout("Stairs: Received signal OK\n");
                break;
        case SIGNAL_PASSENGER_LEFT_STAIRS:
                syncedCout("Stairs: Received signal PASSENGER_LEFT_STAIRS\n");
                break;
        case SIGNAL_STAIRS_OPEN:
                syncedCout("Stairs: Received signal STAIRS_OPEN\n");
                stairsOpen = true;
                break;
        case SIGNAL_STAIRS_CLOSE:
                syncedCout("Stairs: Received signal STAIRS_CLOSE\n");
                stairsOpen = false;
                break;
        case SIGTERM:
                syncedCout("Stairs: Received signal SIGTERM\n");
                exit(0);
                break;
        default:
                syncedCout("Stairs: Received unknown signal\n");
                break;
        }
}

int stairs(int semID1, int semID2)
{
        // attach handler to signals
        struct sigaction sa;
        sa.sa_handler = stairsSignalHandler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;
        if (sigaction(SIGNAL_OK, &sa, NULL) == -1)
        {
                perror("sigaction");
                exit(1);
        }
        if (sigaction(SIGNAL_PASSENGER_LEFT_STAIRS, &sa, NULL) == -1)
        {
                perror("sigaction");
                exit(1);
        }
        if (sigaction(SIGNAL_STAIRS_OPEN, &sa, NULL) == -1)
        {
                perror("sigaction");
                exit(1);
        }
        if (sigaction(SIGNAL_STAIRS_CLOSE, &sa, NULL) == -1)
        {
                perror("sigaction");
                exit(1);
        }
        if (sigaction(SIGTERM, &sa, NULL) == -1)
        {
                perror("sigaction");
                exit(1);
        }

        // 1. queue for stairs (semaphore)
        // 2. stairs open (signal)
        // 3. stairs let passenger in (semaphore) and increment occupancy
        // 4. stairs check if inPlane passengers + onStairs passengers == maxPassengers
        //      4.1. if true, behave like stairs close
        //      4.2. if false, continue letting passengers in
        // 5. stairs check if occupancy == MAX_ALLOWED_OCCUPANCY
        //      5.1. if true, wait for signal SIGNAL_PASSENGER_LEFT_STAIRS
        //      5.2. if false, continue letting passengers in
        // 6. if signal SIGNAL_STAIRS_CLOSE, stop letting passengers in
        // 7. repeat until SIGTERM

        while (true)
        {
                pause(); // wait for signal STAIRS_OPEN
                while (stairsOpen)
                {
                        sembuf inc = {0, 1, 0};
                        sembuf dec = {0, -1, 0};
                        while (semop(semID1, &dec, 1) == -1)
                        {
                                if (errno == EINTR)
                                {
                                        continue;
                                }
                                perror("semop");
                                exit(1);
                        }
                        while (semop(semID2, &inc, 1) == -1)
                        {
                                if (errno == EINTR)
                                {
                                        continue;
                                }
                                perror("semop");
                                exit(1);
                        }
                        planeStairsMutex.lock();
                        stairsOccupancy++;
                        if (stairsOccupancy + passengersOnBoard == PLANE_PLACES)
                        {
                                syncedCout("Stairs: Plane is full\n");
                                break;
                        }
                        if (stairsOccupancy == MAX_ALLOWED_OCCUPANCY)
                        {
                                syncedCout("Stairs: Waiting for passengers to leave\n");
                                pause(); // wait for signal PASSENGER_LEFT_STAIRS
                        }
                        planeStairsMutex.unlock();
                }
        }
}
