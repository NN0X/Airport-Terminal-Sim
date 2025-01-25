#include <iostream>
#include <cstdint>
#include <sys/types.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <errno.h>
#include <mutex>

#include "plane.h"
#include "utils.h"

extern sembuf INC_SEM;
extern sembuf DEC_SEM;

static bool leaveEarly = false;

void stairsSignalHandler(int signum)
{
        switch (signum)
        {
        case SIGNAL_OK:
                vCout("Stairs: Received signal OK\n", MAGENTA, LOG_STAIRS);
                break;
        case SIGNAL_STAIRS_CLOSE:
                vCout("Stairs: Received signal STAIRS_CLOSE\n", MAGENTA, LOG_STAIRS);
                leaveEarly = true;
                break;
        case SIGTERM:
                vCout("Stairs: Received signal SIGTERM\n", MAGENTA, LOG_STAIRS);
                exit(0);
                break;
        default:
                vCout("Stairs: Received unknown signal\n", MAGENTA, LOG_STAIRS);
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
                leaveEarly = false;
                safeSemop(args.semIDStairsWait, &DEC_SEM, 1);
                while (true)
                {
                        safeSemop(args.semIDStairsPassengerIn, &DEC_SEM, 1);
                        safeSemop(args.semIDStairsPassengerWait, &INC_SEM, 1);

                        safeSemop(args.semIDStairsCounter, &DEC_SEM, 1);

                        int stairsOccupancy = STAIRS_MAX_ALLOWED_OCCUPANCY - safeGetSemVal(args.semIDStairsCounter, 0);

                        int passengersOnBoard = safeGetSemVal(args.semIDPlaneCounter, 0);

                        if (stairsOccupancy + passengersOnBoard == PLANE_PLACES)
                        {
                                vCout("Stairs: Plane is full\n", MAGENTA, LOG_STAIRS);
                                break;
                        }
                        if (leaveEarly)
                        {
                                vCout("Stairs: Plane left early\n", MAGENTA, LOG_STAIRS);
                                break;
                        }
                }
        }
}
