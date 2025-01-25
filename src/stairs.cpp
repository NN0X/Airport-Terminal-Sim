#include <iostream>
#include <cstdint>
#include <sys/types.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <errno.h>
#include <mutex>

#include "plane.h"
#include "utils.h"

static bool stairsOpen = false;

extern sembuf INC_SEM;
extern sembuf DEC_SEM;

void stairsSignalHandler(int signum)
{
        switch (signum)
        {
        case SIGNAL_OK:
                vCout("Stairs: Received signal OK\n");
                break;
        case SIGNAL_STAIRS_CLOSE:
                vCout("Stairs: Received signal STAIRS_CLOSE\n");
                stairsOpen = false;
                break;
        case SIGNAL_PASSENGER_LEFT_STAIRS:
                vCout("Stairs: Received signal PASSENGER_LEFT_STAIRS\n");
                break;
        case SIGTERM:
                vCout("Stairs: Received signal SIGTERM\n");
                exit(0);
                break;
        default:
                vCout("Stairs: Received unknown signal\n");
                break;
        }
}

int stairs(StairsArgs args)
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

        while (true)
        {
                stairsOpen = true;
                safeSemop(args.semIDStairsWait, &DEC_SEM, 1);
                while (stairsOpen)
                {
                        usleep(1000); // WARNING: test
                        safeSemop(args.semIDStairsPassengerIn, &DEC_SEM, 1);
                        safeSemop(args.semIDStairsPassengerWait, &INC_SEM, 1);

                        safeSemop(args.semIDStairsCounter, &INC_SEM, 1);

                        int stairsOccupancy = safeGetSemVal(args.semIDStairsCounter, 0);
                        // get occupancy from semaphore
                        int passengersOnBoard = safeGetSemVal(args.semIDPlaneCounter, 0);

                        if (stairsOccupancy + passengersOnBoard == PLANE_PLACES)
                        {
                                vCout("Stairs: Plane is full\n");
                                break;
                        }
                        if (stairsOccupancy == STAIRS_MAX_ALLOWED_OCCUPANCY)
                        {
                                vCout("Stairs: Waiting for passengers to leave\n");
                                pause();
                        }
                }
        }
}
