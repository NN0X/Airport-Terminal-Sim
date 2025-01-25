#include <iostream>
#include <vector>
#include <cstdint>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <mutex>
#include <signal.h>
#include <pthread.h>
#include <sys/sem.h>

#include "utils.h"
#include "args.h"

static bool planeAlreadyInTerminal = false;
static std::vector<pid_t> planesWaiting;

static int semIDStairsWait;
static int semIDPlaneWait;

extern int totalPassengers;

extern sembuf INC_SEM;
extern sembuf DEC_SEM;

void dispatcherSignalHandler(int signum, siginfo_t *info, void *ptr)
{
        pid_t pid;
        switch (signum)
        {
                case SIGNAL_OK:
                        vCout("Dispatcher: Received signal OK\n");
                        break;
                case SIGNAL_PLANE_READY:
                        vCout("Dispatcher: Received signal PLANE_READY\n");
                        pid = info->si_value.sival_int;
                        if (planeAlreadyInTerminal)
                        {
                                vCout("Dispatcher: Plane is already in terminal\n");
                                planesWaiting.push_back(pid);
                                break;
                        }
                        vCout("Dispatcher: Plane " + std::to_string(pid) + " is ready\n");
                        safeSemop(semIDPlaneWait, &INC_SEM, 1);
                        safeSemop(semIDStairsWait, &INC_SEM, 1);
                        planeAlreadyInTerminal = true;
                        break;
                case SIGNAL_PLANE_READY_DEPART:
                        vCout("Dispatcher: Received signal PLANE_READY_DEPART\n");
                        pid = info->si_value.sival_int;
                        vCout("Dispatcher: Plane " + std::to_string(pid) + " is ready to depart\n");
                        kill(pid, SIGNAL_PLANE_GO);
                        planeAlreadyInTerminal = false;
                        if (!planesWaiting.empty())
                        {
                                pid_t nextPlane = planesWaiting.front();
                                planesWaiting.erase(planesWaiting.begin());
                                vCout("Dispatcher: Plane " + std::to_string(nextPlane) + " is ready\n");
                                safeSemop(semIDPlaneWait, &INC_SEM, 1);
                                safeSemop(semIDStairsWait, &INC_SEM, 1);
                                planeAlreadyInTerminal = true;
                        }
                        break;
                case SIGTERM:
                        vCout("Dispatcher: Received signal SIGTERM\n");
                        exit(0);
                        break;
                default:
                        vCout("Dispatcher: Received unknown signal\n");
                        break;
        }
}

void *passengerCounterThread(void *arg)
{
        int semIDPassengerCounter = *((int *)arg);
        int totalPassengersLeft = 0;
        while (true)
        {
                safeSemop(semIDPassengerCounter, &DEC_SEM, 1);
                totalPassengersLeft++;
                std::cout << "Passengers left: " << totalPassengersLeft << " of " << totalPassengers << "\n";
                if (totalPassengersLeft == totalPassengers)
                {
                        vCout("Dispatcher: All passengers left\n");
                        std::cout << "All passengers left\n";
                        // send signal to main to exit all processes
                        exit(0);
                }
        }
}

int dispatcher(DispatcherArgs args)
{
        semIDPlaneWait = args.semIDPlaneWait;
        semIDStairsWait = args.semIDStairsWait;
        // attach handler to signals
        struct sigaction sa;
        sa.sa_sigaction = dispatcherSignalHandler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = SA_SIGINFO;
        if (sigaction(SIGNAL_OK, &sa, NULL) == -1)
        {
                perror("sigaction");
                exit(1);
        }
        if (sigaction(SIGNAL_PLANE_READY, &sa, NULL) == -1)
        {
                perror("sigaction");
                exit(1);
        }
        if (sigaction(SIGNAL_PLANE_READY_DEPART, &sa, NULL) == -1)
        {
                perror("sigaction");
                exit(1);
        }
        if (sigaction(SIGTERM, &sa, NULL) == -1)
        {
                perror("sigaction");
                exit(1);
        }

        pthread_t passengerCounter;
        if (pthread_create(&passengerCounter, NULL, passengerCounterThread, (void *)&args.semIDPassengerCounter) != 0)
        {
                perror("pthread_create");
                exit(1);
        }

        while (true)
        {
                pause();
        }
}
