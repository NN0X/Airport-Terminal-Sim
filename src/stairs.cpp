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
                while (semop(args.semIDStairsWait, &DEC_SEM, 1) == -1)
                {
                        if (errno == EINTR)
                        {
                                continue;
                        }
                        perror("semop");
                        exit(1);
                }
                while (stairsOpen)
                {
                        usleep(1000); // WARNING: test
                        std::cout << "Stairs: Waiting for passengers\n";
                        while (semop(args.semIDStairsPassengerIn, &DEC_SEM, 1) == -1)
                        {
                                if (errno == EINTR)
                                {
                                        continue;
                                }
                                perror("semop");
                                exit(1);
                        }
                        while (semop(args.semIDStairsPassengerWait, &INC_SEM, 1) == -1)
                        {
                                if (errno == EINTR)
                                {
                                        continue;
                                }
                                perror("semop");
                                exit(1);
                        }

                        while (semop(args.semIDStairsCounter, &INC_SEM, 1) == -1)
                        {
                                if (errno == EINTR)
                                {
                                        continue;
                                }
                                perror("semop");
                                exit(1);
                        }

                        int stairsOccupancy;
                        // get occupancy from semaphore
                        while ((stairsOccupancy = semctl(args.semIDStairsCounter, 0, GETVAL)) == -1)
                        {
                                if (errno == EINTR)
                                {
                                        continue;
                                }
                                perror("semctl");
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

                        if (stairsOccupancy + passengersOnBoard == PLANE_PLACES)
                        {
                                vCout("Stairs: Plane is full\n");
                                // set semaphore to 0
                                while (semctl(args.semIDStairsCounter, 0, SETVAL, 0) == -1)
                                {
                                        if (errno == EINTR)
                                        {
                                                continue;
                                        }
                                        perror("semctl");
                                        exit(1);
                                }
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
