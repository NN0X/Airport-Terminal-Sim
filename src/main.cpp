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
#include <random>

#include "plane.h"
#include "passenger.h"

struct ThreadArgs
{
        uint64_t id;
        int semIDs[2];
};

std::mutex coutMutex;

#define VERBOSE 1
#define MAX_PASSENGER_DELAY 10000 // in ticks

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

        Passenger passenger(args->id);

        // TODO: run passenger tasks here

        // INFO: passenger waits for security control to be free
        // INFO: passenger waits for plane to be ready
        // INFO: passenger thread ends

        pthread_exit(NULL);
}

std::vector<pthread_t> initPassengers(size_t num, int *semIDs, const std::vector<uint64_t> &delays)
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
                syncedCout("Waiting for " + std::to_string(delays[i]) + " ticks\n");
                for(uint64_t j = 0; j < delays[i]; j++) {}
        }

        syncedCout("All passengers created\n");

        return threads;
}

void *planeThread(void *arg)
{
        ThreadArgs *args = (ThreadArgs *)arg;

        syncedCout("Plane thread: " + std::to_string(args->id) + "\n");

        Plane plane(args->id);

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

        // INFO: plane waits for terminal to be free
        // INFO: plane waits for passengers to board
        // INFO: plane waits x time
        // INFO: repeat until signal to exit

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

void *secGateThread(void *arg)
{
}

int secControl()
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

int gate()
{
        while (true)
        {
                // INFO: queue of passengers waiting to board
                // INFO: wait for signal to let passengers board and open gate
                // INFO: signal dispatcher when N passengers are pass the gate and close gate
                // INFO: repeat until signal to exit
        }
}

int stairs()
{
        while (true)
        {
                // INFO: this is only a complication point for simulating communication delays between dispatcher and plane
                // INFO: sends queue from gate to plane
                // INFO: repeat until signal to exit
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

void genRandomVector(std::vector<uint64_t> &vec, uint64_t min, uint64_t max)
{
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<uint64_t> dis(min, max);

        for (size_t i = 0; i < vec.size(); i++)
        {
                vec[i] = dis(gen);
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

        int semIDsPlanes[2];

        semIDsPlanes[0] = semget(IPC_PRIVATE, 1, IPC_CREAT | 0666);
        semIDsPlanes[1] = semget(IPC_PRIVATE, 1, IPC_CREAT | 0666);

        if (semIDsPlanes[0] == -1 || semIDsPlanes[1] == -1)
        {
                perror("semget");
                exit(1);
        }

        semctl(semIDsPlanes[0], 0, SETVAL, numPlanes);
        semctl(semIDsPlanes[1], 0, SETVAL, 0);

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
                syncedCout("Main process: " + std::to_string(getpid()) + " waiting for planes to init\n");

                sembuf buf = {0, 0, 0};
                if (semop(semIDsPlanes[0], &buf, 1) == -1)
                {
                        perror("semop");
                        exit(1);
                }

                buf = {0, 1, 0};
                for (size_t i = 0; i < numPlanes; i++)
                {
                        if (semop(semIDsPlanes[1], &buf, 1) == -1)
                        {
                                perror("semop");
                                exit(1);
                        }
                }

                syncedCout("Main process: " + std::to_string(getpid()) + " released\n");

                // TODO: runtime

                // TODO: move passenger spawning to another process
                std::vector<uint64_t> delays(numPassengers);
                genRandomVector(delays, 0, MAX_PASSENGER_DELAY);
                std::vector<pthread_t> passengerThreads = initPassengers(numPassengers, semIDsPlanes, delays);

                // cleanup

                for (pthread_t thread : passengerThreads)
                {
                        if (pthread_join(thread, NULL) != 0)
                        {
                                perror("pthread_join");
                                exit(1);
                        }
                }
                syncedCout("All passengers finished\n");
                for (pthread_t thread : planeThreads)
                {
                        if (pthread_join(thread, NULL) != 0)
                        {
                                perror("pthread_join");
                                exit(1);
                        }
                }
                syncedCout("All planes finished\n");
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


