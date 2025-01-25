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
static pid_t planeInTerminal;

static int semIDStairsWait;
static int semIDPlaneWait;
static int semIDPlaneDepart;

extern int totalPassengers;

extern sembuf INC_SEM;
extern sembuf DEC_SEM;

std::mutex dispatcherMutex;

void dispatcherSignalHandler(int signum, siginfo_t *info, void *ptr)
{
        pid_t pid;
        switch (signum)
        {
                case SIGNAL_OK:
                        vCout("Dispatcher: Received signal OK\n", RED, LOG_DISPATCHER);
                        break;
                case SIGNAL_PLANE_READY:
                        vCout("Dispatcher: Received signal PLANE_READY\n", RED, LOG_DISPATCHER);
                        pid = info->si_value.sival_int;
                        dispatcherMutex.lock();
                        if (planeAlreadyInTerminal)
                        {
                                vCout("Dispatcher: Plane is already in terminal\n", RED, LOG_DISPATCHER);
                                planesWaiting.push_back(pid);
                                dispatcherMutex.unlock();
                                break;
                        }
                        vCout("Dispatcher: Plane " + std::to_string(pid) + " is ready\n", RED, LOG_DISPATCHER);
                        safeSemop(semIDPlaneWait, &INC_SEM, 1);
                        safeSemop(semIDStairsWait, &INC_SEM, 1);
                        planeAlreadyInTerminal = true;
                        planeInTerminal = pid;
                        dispatcherMutex.unlock();
                        break;
                case SIGNAL_PLANE_READY_DEPART:
                        vCout("Dispatcher: Received signal PLANE_READY_DEPART\n", RED, LOG_DISPATCHER);
                        pid = info->si_value.sival_int;
                        vCout("Dispatcher: Plane " + std::to_string(pid) + " is ready to depart\n", RED, LOG_DISPATCHER);
                        kill(pid, SIGNAL_PLANE_GO);
                        dispatcherMutex.lock();
                        planeAlreadyInTerminal = false;
                        if (!planesWaiting.empty())
                        {
                                pid_t nextPlane = planesWaiting.front();
                                planesWaiting.erase(planesWaiting.begin());
                                vCout("Dispatcher: Plane " + std::to_string(nextPlane) + " is ready\n", RED, LOG_DISPATCHER);
                                safeSemop(semIDPlaneWait, &INC_SEM, 1);
                                safeSemop(semIDStairsWait, &INC_SEM, 1);
                                planeAlreadyInTerminal = true;
                                planeInTerminal = nextPlane;
                        }
                        safeSemop(semIDPlaneDepart, &INC_SEM, 1);
                        dispatcherMutex.unlock();
                        break;
                case SIGNAL_DISPATCHER_PLANE_FORCED_DEPART:
                        vCout("Dispatcher: Received signal DISPATCHER_PLANE_FORCED_DEPART\n", RED, LOG_DISPATCHER);
                        dispatcherMutex.lock();
                        vCout("Dispatcher: Plane " + std::to_string(planeInTerminal) + " is forced to depart\n", RED, LOG_DISPATCHER);
                        kill(planeInTerminal, SIGNAL_PLANE_GO);
                        planeAlreadyInTerminal = false;
                        if (!planesWaiting.empty())
                        {
                                pid_t nextPlane = planesWaiting.front();
                                planesWaiting.erase(planesWaiting.begin());
                                vCout("Dispatcher: Plane " + std::to_string(nextPlane) + " is ready\n", RED, LOG_DISPATCHER);
                                safeSemop(semIDPlaneWait, &INC_SEM, 1);
                                safeSemop(semIDStairsWait, &INC_SEM, 1);
                                planeAlreadyInTerminal = true;
                        }
                        safeSemop(semIDPlaneDepart, &INC_SEM, 1);
                        dispatcherMutex.unlock();
                        break;
                case SIGTERM:
                        vCout("Dispatcher: Received signal SIGTERM\n", RED, LOG_DISPATCHER);
                        exit(0);
                        break;
                default:
                        vCout("Dispatcher: Received unknown signal\n", RED, LOG_DISPATCHER);
                        break;
        }
}

void *passengerCounterThread(void *arg)
{
        int *args = (int *)arg;
        int semIDPassengerCounter = args[0];
        pid_t pidMain = args[1];
        int totalPassengersLeft = 0;
        while (true)
        {
                safeSemop(semIDPassengerCounter, &DEC_SEM, 1);
                totalPassengersLeft++;
                vCout("Dispatcher: Passengers left: " + std::to_string(totalPassengersLeft) + "/" + std::to_string(totalPassengers) + "\n", RED, LOG_DISPATCHER);
                if (totalPassengersLeft == totalPassengers)
                {
                        vCout("Dispatcher: All passengers left\n", RED, LOG_DISPATCHER);
                        kill(pidMain, SIGNAL_THIS_IS_THE_END_HOLD_YOUR_BREATH_AND_COUNT_TO_TEN);
                }
        }
}

int dispatcher(DispatcherArgs args)
{
        semIDPlaneWait = args.semIDPlaneWait;
        semIDStairsWait = args.semIDStairsWait;
        semIDPlaneDepart = args.semIDPlaneDepart;
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
        if (sigaction(SIGNAL_DISPATCHER_PLANE_FORCED_DEPART, &sa, NULL) == -1)
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
        int argsPassengerCounter[2] = {args.semIDPassengerCounter, args.pidMain};

        if (pthread_create(&passengerCounter, NULL, passengerCounterThread, (void *)argsPassengerCounter) != 0)
        {
                perror("pthread_create");
                exit(1);
        }

        while (true)
        {
                pause();
        }
}
