#include <iostream>
#include <vector>
#include <cstdint>
#include <unistd.h>
#include <sys/types.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <errno.h>
#include <pthread.h>
#include <mutex>

#include "plane.h"
#include "passenger.h"

struct ThreadArgs
{
        uint64_t id;
        int semIDs[2];
};

std::mutex coutMutex;

#define VERBOSE 1

void syncedCout(std::string msg)
{
        if (VERBOSE == 0)
                return;
        std::lock_guard<std::mutex> lock(coutMutex);
        std::cout << msg;
}

void *passengerThread(void *arg)
{
        ThreadArgs *args = (ThreadArgs *)arg;

        syncedCout("Passenger thread: " + std::to_string(args->id) + "\n");

        // decrement semaphore value by 1
        sembuf buf = {0, -1, 0};
        if (semop(args->semIDs[0], &buf, 1) == -1)
        {
                perror("semop");
                pthread_exit(NULL);
        }
        syncedCout("Semaphore decremented\nPassenger thread: " + std::to_string(args->id) + " waiting for semaphore 1\n");
        // wait for semaphore 1 to be incremented by 1
        if (semop(args->semIDs[1], &buf, 1) == -1)
        {
                perror("semop");
                pthread_exit(NULL);
        }
        syncedCout("Passenger thread: " + std::to_string(args->id) + " released\n");

        // TODO: run passenger tasks here

        // TODO: passenger waits for security control to be free
        // TODO: passenger waits for plane to be ready
        // TODO: passenger thread ends

        pthread_exit(NULL);
}

std::vector<pthread_t> initPassengers(size_t num, int *semIDs)
{
        std::vector<pthread_t> threads(num);
        for (size_t i = 0; i < num; i++)
        {
                ThreadArgs *args = new ThreadArgs;
                args->id = i;
                args->semIDs[0] = semIDs[0];
                args->semIDs[1] = semIDs[1];

                if (pthread_create(&threads[i], NULL, passengerThread, args) != 0)
                {
                        perror("pthread_create");
                        exit(1);
                }
        }

        return threads;
}

void *planeThread(void *arg)
{
        ThreadArgs *args = (ThreadArgs *)arg;

        syncedCout("Plane thread: " + std::to_string(args->id) + "\n");

        // decrement semaphore value by 1
        sembuf buf = {0, -1, 0};
        if (semop(args->semIDs[0], &buf, 1) == -1)
        {
                perror("semop");
                pthread_exit(NULL);
        }
        syncedCout("Semaphore decremented\nPlane thread: " + std::to_string(args->id) + " waiting for semaphore 1\n");
        // wait for semaphore 1 to be incremented by 1
        if (semop(args->semIDs[1], &buf, 1) == -1)
        {
                perror("semop");
                pthread_exit(NULL);
        }
        syncedCout("Plane thread: " + std::to_string(args->id) + " released\n");

        // TODO: run plane tasks here

        // TODO: plane waits for terminal to be free
        // TODO: plane waits for passengers to board
        // TODO: plane waits x time
        // TODO: repeat until signal to exit

        pthread_exit(NULL);
}

std::vector<pthread_t> initPlanes(size_t num, int *semIDs)
{
        std::vector<pthread_t> threads(num);
        for (size_t i = 0; i < num; i++)
        {
                ThreadArgs *args = new ThreadArgs;
                args->id = i;
                args->semIDs[0] = semIDs[0];
                args->semIDs[1] = semIDs[1];

                if (pthread_create(&threads[i], NULL, planeThread, args) != 0)
                {
                        perror("pthread_create");
                        exit(1);
                }
        }

        return threads;
}

int baggageControl()
{
        while (true)
        {
        }
}

int secControl()
{
        while (true)
        {
                // TODO: routes passengers to 3 different gates from one queue
                // TODO: max 2 passengers per gate (same gender)
                // TODO: count passengers passing through each gate and when N passengers pass through, signal dispatcher
                // TODO: repeat until queue is empty
        }
}

int dispatcher()
{
        while (true)
        {
                // TODO: signal plane to go to terminal
                // TODO: signal plane to wait for passengers if queue is not empty
                // TODO: signal gate to let passengers board
                // TODO: signal plane when passengers are on board (on stairs)
                // TODO: repeat until signal to exit
        }
}

int gate()
{
        while (true)
        {
                // TODO: queue of passengers waiting to board
                // TODO: wait for signal to let passengers board and open gate
                // TODO: signal dispatcher when N passengers are pass the gate and close gate
                // TODO: repeat until signal to exit
        }
}

int stairs()
{
        while (true)
        {
                // TODO: this is only a complication point for simulating communication delays between dispatcher and plane
                // TODO: sends queue from gate to plane
                // TODO: repeat until signal to exit
        }
}

int eventGenerator()
{
        while (true)
        {
                // INFO: generates random events without input from other processes
                // INFO: sends signals to eventHandler
        }
}

int randomEvent(uint8_t category)
{
        while (true)
        {
                // INFO: generates random events
        }
}

int eventHandler()
{
        while (true)
        {
                // INFO: receives signals from other processes and threads
                // INFO: based on signal, it generates random event of a specific category
                // INFO: sends signals to other processes and threads based on event
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

        std::vector<pid_t> pids(3);
        pids[0] = getpid();

        // main + secControl + dispatcher + n passengers + n planes

        size_t numPassengers = std::stoul(argv[1]);
        size_t numPlanes = std::stoul(argv[2]);

        if (numPassengers > 32767 || numPlanes > 32767)
        {
                std::cerr << "Maximum values: <numPassengers> = 32767, <numPlanes> = 32767\n";
                return 1;
        }

        int semIDsPassengers[2];
        int semIDsPlanes[2];

        semIDsPassengers[0] = semget(IPC_PRIVATE, 1, IPC_CREAT | 0666);
        semIDsPassengers[1] = semget(IPC_PRIVATE, 1, IPC_CREAT | 0666);
        semIDsPlanes[0] = semget(IPC_PRIVATE, 1, IPC_CREAT | 0666);
        semIDsPlanes[1] = semget(IPC_PRIVATE, 1, IPC_CREAT | 0666);

        if (semIDsPassengers[0] == -1 || semIDsPassengers[1] == -1 || semIDsPlanes[0] == -1 || semIDsPlanes[1] == -1)
        {
                perror("semget");
                exit(1);
        }

        semctl(semIDsPassengers[0], 0, SETVAL, numPassengers);
        semctl(semIDsPassengers[1], 0, SETVAL, 0);
        semctl(semIDsPlanes[0], 0, SETVAL, numPlanes);
        semctl(semIDsPlanes[1], 0, SETVAL, 0);

        std::vector<pthread_t> passengerThreads = initPassengers(numPassengers, semIDsPassengers);
        std::vector<pthread_t> planeThreads = initPlanes(numPlanes, semIDsPlanes);

        fork();
        pid_t pid = getpid();
        if (pid != pids[0])
        {
                pids[1] = pid;
                fork();
                pid = getpid();
                if (pid != pids[0] && pid != pids[1])
                {
                        pids[2] = pid;
                }
        }

        if (pid == pids[0])
        {
                syncedCout("Main process: " + std::to_string(getpid()) + " waiting for passengers to init\n");
                sembuf buf = {0, 0, 0};
                if (semop(semIDsPassengers[0], &buf, 1) == -1)
                {
                        perror("semop");
                        exit(1);
                }
                syncedCout("Main process: " + std::to_string(getpid()) + " waiting for planes to init\n");

                if (semop(semIDsPlanes[0], &buf, 1) == -1)
                {
                        perror("semop");
                        exit(1);
                }

                // increment semaphore 1 by 1 * numPassengers
                for (size_t i = 0; i < numPassengers; i++)
                {
                        sembuf buf = {0, 1, 0};
                        if (semop(semIDsPassengers[1], &buf, 1) == -1)
                        {
                                perror("semop");
                                exit(1);
                        }
                }

                // increment semaphore 3 by 1 * numPlanes
                for (size_t i = 0; i < numPlanes; i++)
                {
                        sembuf buf = {0, 1, 0};
                        if (semop(semIDsPlanes[1], &buf, 1) == -1)
                        {
                                perror("semop");
                                exit(1);
                        }
                }

                syncedCout("Main process: " + std::to_string(getpid()) + " released\n");

                // TODO: runtime


                // cleanup

                for (pthread_t thread : passengerThreads)
                {
                        if (pthread_join(thread, NULL) != 0)
                        {
                                perror("pthread_join");
                                exit(1);
                        }
                }
                for (pthread_t thread : planeThreads)
                {
                        if (pthread_join(thread, NULL) != 0)
                        {
                                perror("pthread_join");
                                exit(1);
                        }
                }

                if (semctl(semIDsPassengers[0], 0, IPC_RMID) == -1)
                {
                        perror("semctl");
                        exit(1);
                }
                if (semctl(semIDsPassengers[1], 0, IPC_RMID) == -1)
                {
                        perror("semctl");
                        exit(1);
                }
                if (semctl(semIDsPlanes[0], 0, IPC_RMID) == -1)
                {
                        perror("semctl");
                        exit(1);
                }
                if (semctl(semIDsPlanes[1], 0, IPC_RMID) == -1)
                {
                        perror("semctl");
                        exit(1);
                }
        }


        // testing
        if (getpid() == pids[0])
                std::cout << "Main process: " << getpid() << std::endl;
        else if (getpid() == pids[1])
                std::cout << "SecControl process: " << getpid() << std::endl;
        else if (getpid() == pids[2])
                std::cout << "Dispatcher process: " << getpid() << std::endl;

        return 0;
}
