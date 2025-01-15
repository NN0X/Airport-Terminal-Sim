#include <iostream>
#include <vector>
#include <cstdint>
#include <unistd.h>
#include <sys/types.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <sys/prctl.h>
#include <errno.h>
#include <pthread.h>
#include <mutex>
#include <random>
#include <string>

#include "plane.h"
#include "passenger.h"
#include "events.h"
#include "utils.h"

// INFO: can use fifo and semaphores for totality of communication

// FIX: there is an unknown process in the switch statement. find out what it is

#define MAX_BAGGAGE_WEIGHT 200 // kg

int baggageControl(int semID)
{
        // INFO: check if passenger baggage weight is within limits
        // INFO: if not, signal event handler
        // INFO: repeat until signal to exit
        while (true)
        {
        }
}

void *secGateThread(void *arg)
{
}

int secControl(int semID)
{
        // INFO: each sec gate is a separate thread
        while (true)
        {
                // INFO: routes passengers to 3 different gates from one queue
                // INFO: max 2 passengers per gate (same gender)
                // INFO: count passengers passing through each gate and when N passengers pass through, signal dispatcher
                // INFO: repeat until queue is empty
        }
}

int dispatcher()
{
        while (true)
        {
                // INFO: signal plane to go to terminal
                // INFO: signal plane to wait for passengers if queue is not empty
                // INFO: signal gate to let passengers board
                // INFO: signal plane when passengers are on board (on stairs)
                // INFO: repeat until signal to exit
        }
}

int stairs(int semIDGate, int semIDPlaneStairs)
{
        while (true)
        {
                // INFO: queue of passengers waiting to board
                // INFO: wait for signal to let passengers board and open gate
                // INFO: signal dispatcher when N passengers are pass the gate and close gate
                // INFO: max passengers on stairs is lower than max passengers on plane
                // INFO: sends queue from gate to plane
                // INFO: repeat until signal to exit
        }
}

int main(int argc, char* argv[])
{
        // TODO: consider dynamic addition of semaphores based on number of passengers and planes
        // TODO: check system limits for threads
        // TODO: consider rewriting to std threads

        if (argc != 3)
        {
                std::cerr << "Usage: " << argv[0] << " <numPassengers> <numPlanes>\n";
                return 1;
        }

        std::vector<pid_t> pids(1);
        pids[MAIN] = getpid();

        // main + secControl + dispatcher + n passengers + n planes

        size_t numPassengers = std::stoul(argv[1]);
        size_t numPlanes = std::stoul(argv[2]);

        if (numPassengers > 32767 || numPlanes > 32767)
        {
                std::cerr << "Maximum values: <numPassengers> = 32767, <numPlanes> = 32767\n";
                return 1;
        }

        std::vector<int> semIDs = initSemaphores(0666);

        initPlanes(numPlanes, semIDs[PLANE_STAIRS]);

        std::vector<std::string> names = {"baggageControl", "secControl", "dispatcher", "stairs"};
        createSubprocesses(4, pids, names);

        if (getpid() == pids[MAIN])
        {
                // TODO: runtime

                std::vector<uint64_t> delays(numPassengers);
                genRandomVector(delays, 0, MAX_PASSENGER_DELAY);

                createSubprocesses(1, pids, {"spawnPassengers"});
                if (getpid() != pids[MAIN])
                {
                        spawnPassengers(numPassengers, delays, semIDs[BAGGAGE_CTRL], semIDs[SEC_GATE], semIDs[GATE]);
                }

                while (true)
                {
                        // INFO: wait for all processes to finish

                        if (getc(stdin) == 'q')
                        {
                                break;
                        }
                }

                // INFO: cleanup
                // INFO: exit

        }

        switch (getpid())
        {
                case MAIN:
                        break;
                case BAGGAGE_CONTROL:
                        baggageControl(semIDs[BAGGAGE_CTRL]);
                        break;
                case SEC_CONTROL:
                        secControl(semIDs[SEC_GATE]);
                        break;
                case DISPATCHER:
                        dispatcher();
                        break;
                case STAIRS:
                        stairs(semIDs[GATE], semIDs[PLANE_STAIRS]);
                        break;
                default:
                        std::cerr << "Unknown process\n";
                        break;
        }


        // debug process identification
        pid_t pid = getpid();
        if (pid == pids[MAIN])
        {
                std::cout << "Main process\n";
        }
        else if (pid == pids[BAGGAGE_CONTROL])
        {
                std::cout << "Baggage control process\n";
        }
        else if (pid == pids[SEC_CONTROL])
        {
                std::cout << "Security control process\n";
        }
        else if (pid == pids[DISPATCHER])
        {
                std::cout << "Dispatcher process\n";
        }
        else if (pid == pids[STAIRS])
        {
                std::cout << "Stairs process\n";
        }
        else if (pid == pids[STAIRS + 1])
        {
                std::cout << "Spawn passengers process\n";
        }
        else
        {
                std::cerr << "Unknown process\n";
        }

        return 0;
}


