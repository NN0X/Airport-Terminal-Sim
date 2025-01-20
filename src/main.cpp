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
#include <fcntl.h> // O_RDONLY, O_WRONLY
#include <sys/stat.h> // mkfifo
#include <signal.h>

#include "plane.h"
#include "passenger.h"
#include "baggageControl.h"
#include "secControl.h"
#include "events.h"
#include "dispatcher.h"
#include "stairs.h"
#include "utils.h"

// TODO: change spawnPassengers and initPlanes to return pid_t
// FIX: initPlanes (creates wrong number of planes)
// TODO: send sigterm to all processes not just baggage control

int main(int argc, char* argv[])
{
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

        std::vector<std::string> names = {"baggageControl", "secControl", "dispatcher", "stairs"};
        createSubprocesses(4, pids, names);

        initPlanes(numPlanes, semIDs[PLANE_STAIRS], pids[STAIRS], pids[DISPATCHER]);

        pid_t currPid = getpid();

        if (currPid == pids[MAIN])
        {
                std::cout << "Main process\n";
                // TODO: runtime

                std::vector<uint64_t> delays(numPassengers);
                genRandomVector(delays, 0, MAX_PASSENGER_DELAY);

                createSubprocesses(1, pids, {"spawnPassengers"});
                if (getpid() != pids[MAIN])
                {
                        spawnPassengers(numPassengers, delays, semIDs[BAGGAGE_CTRL], semIDs[SEC_CTRL], {semIDs[SEC_GATE_0], semIDs[SEC_GATE_1], semIDs[SEC_GATE_2]});
                }

                while (true)
                {
                        // INFO: wait for signal to exit
                        if (getc(stdin) == 'q')
                        {
                                for (size_t i = 1; i < pids.size(); i++)
                                {
                                        kill(pids[i], SIGTERM);
                                }
                                break;
                        }
                }
                // INFO: cleanup

                // delete semaphores
                for (size_t i = 0; i < semIDs.size(); i++)
                {
                        if (semctl(semIDs[i], 0, IPC_RMID) == -1)
                        {
                                perror("semctl");
                                exit(1);
                        }
                }
                // delete fifo
                if (unlink("baggageControlFIFO") == -1)
                {
                        perror("unlink");
                        exit(1);
                }
                vCout("Main process: Exiting\n");
        }
        else if (currPid == pids[BAGGAGE_CONTROL])
        {
                std::cout << "Baggage control process\n";
                baggageControl(semIDs[BAGGAGE_CTRL]);
        }
        else if (currPid == pids[SEC_CONTROL])
        {
                std::cout << "Security control process\n";
                secControl(semIDs[SEC_CTRL], semIDs[SEC_GATE_0], semIDs[SEC_GATE_1], semIDs[SEC_GATE_2]);
        }
        else if (currPid == pids[DISPATCHER])
        {
                std::cout << "Dispatcher process\n";
                dispatcher({}, pids[STAIRS]);
        }
        else if (currPid == pids[STAIRS])
        {
                std::cout << "Stairs process\n";
                stairs(semIDs[STAIRS_QUEUE]);
        }
        else
        {
                std::cerr << "Unknown process\n";
        }

        return 0;
}


