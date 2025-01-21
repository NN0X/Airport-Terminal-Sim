#include <iostream>
#include <vector>
#include <cstdint>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <mutex>
#include <signal.h>

#include "utils.h"

static pid_t stairsPID;
static bool planeAlreadyInTerminal = false;
static std::vector<pid_t> planesWaiting;

int totalPassengersLeft = 0;

extern int totalPassengers;

void dispatcherSignalHandler(int signum, siginfo_t *info, void *ptr)
{
        pid_t pid;
        switch (signum)
        {
        case SIGNAL_OK:
                syncedCout("Dispatcher: Received signal OK\n");
                break;
        case SIGNAL_PLANE_READY:
                syncedCout("Dispatcher: Received signal PLANE_READY\n");
                pid = info->si_value.sival_int;
                if (planeAlreadyInTerminal)
                {
                        syncedCout("Dispatcher: Plane is already in terminal\n");
                        planesWaiting.push_back(pid);
                        break;
                }
                syncedCout("Dispatcher: Plane " + std::to_string(pid) + " is ready\n");
                kill(pid, SIGNAL_PLANE_RECEIVE);
                kill(stairsPID, SIGNAL_STAIRS_OPEN);
                planeAlreadyInTerminal = true;
                break;
        case SIGNAL_PLANE_READY_DEPART:
                syncedCout("Dispatcher: Received signal PLANE_READY_DEPART\n");
                pid = info->si_value.sival_int;
                syncedCout("Dispatcher: Plane " + std::to_string(pid) + " is ready to depart\n");
                kill(pid, SIGNAL_PLANE_GO);
                planeAlreadyInTerminal = false;
                if (!planesWaiting.empty())
                {
                        pid_t nextPlane = planesWaiting.front();
                        planesWaiting.erase(planesWaiting.begin());
                        syncedCout("Dispatcher: Plane " + std::to_string(nextPlane) + " is ready\n");
                        kill(nextPlane, SIGNAL_PLANE_RECEIVE);
                        kill(stairsPID, SIGNAL_STAIRS_OPEN);
                        planeAlreadyInTerminal = true;
                }
                break;
        case SIGNAL_PASSENGER_LEFT:
                syncedCout("Dispatcher: Received signal PASSENGER_LEFT\n");
                totalPassengersLeft++;
                if (totalPassengersLeft == totalPassengers)
                {
                        syncedCout("Dispatcher: All passengers left\n");
                        // send signal to main to exit all processes
                        totalPassengersLeft = 0;
                }
                break;
        case SIGTERM:
                syncedCout("Dispatcher: Received signal SIGTERM\n");
                exit(0);
                break;
        default:
                syncedCout("Dispatcher: Received unknown signal\n");
                break;
        }
}

int dispatcher(pid_t stairsPid)
{
        stairsPID = stairsPid;
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
        if (sigaction(SIGNAL_PASSENGER_LEFT, &sa, NULL) == -1)
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
                pause();
        }
}
