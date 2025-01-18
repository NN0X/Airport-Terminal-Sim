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
#include "events.h"
#include "utils.h"

// INFO: can use fifo and semaphores for totality of communication

// TODO: send sigterm to all processes not just baggage control
// TODO: cleanup baggage control and passenger

void baggageControlSignalHandler(int signum)
{
        switch (signum)
        {
                case SIGNAL_OK:
                        syncedCout("Baggage control: Received signal OK\n");
                        break;
                case SIGTERM:
                        syncedCout("Baggage control: Received signal SIGTERM\n");
                        exit(0);
                default:
                        syncedCout("Baggage control: Received unknown signal\n");
                        break;
        }
}

int baggageControl(int semID)
{
        // INFO: check if passenger baggage weight is within limits
        // INFO: if not, signal event handler
        // INFO: repeat until signal to exit

        // check if fifo exists
        if (access("baggageControlFIFO", F_OK) == -1)
        {
                if (mkfifo("baggageControlFIFO", 0666) == -1)
                {
                        perror("mkfifo");
                        exit(1);
                }
        }

        // set signal handler
        struct sigaction sa;
        sa.sa_handler = baggageControlSignalHandler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;
        if (sigaction(SIGNAL_OK, &sa, NULL) == -1)
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
                // increment semaphore
                syncedCout("Baggage control: incrementing semaphore\n");
                sembuf inc = {0, 1, 0};
                if (semop(semID, &inc, 1) == -1)
                {
                        perror("semop");
                        exit(1);
                }
                int fd = open("baggageControlFIFO", O_RDONLY);
                if (fd == -1)
                {
                        perror("open");
                        exit(1);
                }

                // read from fifo the passenger pid and baggage weight
                BaggageInfo baggageInfo;
                if (read(fd, &baggageInfo, sizeof(baggageInfo)) == -1)
                {
                        perror("read");
                        exit(1);
                }
                close(fd);
                syncedCout("Baggage control: received baggage info\n");

                if (baggageInfo.mBaggageWeight > MAX_ALLOWED_BAGGAGE_WEIGHT)
                {
                        syncedCout("Baggage control: passenger " + std::to_string(baggageInfo.mPid) + " is overweight\n");
                        kill(baggageInfo.mPid, PASSENGER_IS_OVERWEIGHT);
                        // TODO: consider sending PASSENGER_IS_OVERWEIGHT signal to event handler
                        // and then event handler sending signal to passenger
                }
                else
                {
                        syncedCout("Baggage control: passenger " + std::to_string(baggageInfo.mPid) + " is not overweight\n");
                        // send empty signal to unblock passenger
                        kill(baggageInfo.mPid, SIGNAL_OK);
                }
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
                        spawnPassengers(numPassengers, delays, semIDs[BAGGAGE_CTRL], semIDs[SEC_GATE], semIDs[GATE]);
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
                secControl(semIDs[SEC_GATE]);
        }
        else if (currPid == pids[DISPATCHER])
        {
                std::cout << "Dispatcher process\n";
                dispatcher();
        }
        else if (currPid == pids[STAIRS])
        {
                std::cout << "Stairs process\n";
                stairs(semIDs[GATE], semIDs[PLANE_STAIRS]);
        }
        else
        {
                std::cerr << "Unknown process\n";
        }

        return 0;
}


