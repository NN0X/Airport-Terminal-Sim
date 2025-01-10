#include <iostream>
#include <vector>
#include <cstdint>
#include <unistd.h>
#include <sys/types.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <errno.h>

#include "plane.h"
#include "passenger.h"

void initPassengers(size_t num, pid_t mainPID, int *semIDs)
{
        pid_t prev = mainPID;
        for (size_t i = 0; i < num; ++i)
        {
                fork();
                pid_t pid = getpid();
                if (pid != prev)
                {
                        prev = pid;
                }
                else
                {
                        break;
                }

        }

        if (getpid() == mainPID)
        {
                return;
        }

        std::cout << "Passenger process: " << getpid() << "\n";

        // decrement semaphore value by 1
        sembuf buf = {0, -1, 0};
        if (semop(semIDs[0], &buf, 1) == -1)
        {
                perror("semop");
                exit(1);
        }
        std::cout << "Semaphore decremented" << std::endl;
        std::cout << "Passenger process: " << getpid() << " waiting for semaphore 1\n";
        // wait for semaphore 1 to be incremented by 1
        if (semop(semIDs[1], &buf, 1) == -1)
        {
                perror("semop");
                exit(1);
        }

        std::cout << "Passenger process: " << getpid() << " released\n";

        // TODO: run passenger tasks here

        // TODO: passenger waits for security control to be free
        // TODO: passenger waits for plane to be ready
        // TODO: passenger process ends

        if (getpid() != mainPID)
        {
                exit(0);
        }
}

void initPlanes(size_t num, pid_t mainPID, int *semIDs)
{
        pid_t prev = mainPID;
        for (size_t i = 0; i < num; ++i)
        {
                fork();
                pid_t pid = getpid();
                if (pid != prev)
                {
                        prev = pid;
                }
                else
                {
                        break;
                }
        }

        if (getpid() == mainPID)
        {
                return;
        }

        std::cout << "Plane process: " << getpid() << "\n";

        // decrement semaphore value by 1
        sembuf buf = {0, -1, 0};
        if (semop(semIDs[0], &buf, 1) == -1)
        {
                perror("semop");
                exit(1);
        }
        std::cout << "Semaphore decremented" << std::endl;
        std::cout << "Plane process: " << getpid() << " waiting for semaphore 1\n";
        // wait for semaphore 1 to be incremented by 1
        if (semop(semIDs[1], &buf, 1) == -1)
        {
                perror("semop");
                exit(1);
        }

        std::cout << "Plane process: " << getpid() << " released\n";

        // TODO: run plane tasks here

        // TODO: plane waits for terminal to be free
        // TODO: plane waits for passengers to board
        // TODO: plane waits x time
        // TODO: repeat until signal to exit

        if (getpid() != mainPID)
        {
                exit(0);
        }
}

int secControl()
{
        while (true)
        {
                // INFO: each passenger can wait at most 3 times e.g.

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
                // TODO: signal plane when passengers are on board
                // TODO: repeat until signal to exit
        }
}

int main(int argc, char* argv[])
{
        // TODO: checks for argc and argv


        std::vector<pid_t> pids(3);
        pids[0] = getpid();

        // main + secControl + dispatcher + n passengers + n planes

        size_t numPassengers = std::stoul(argv[1]);
        size_t numPlanes = std::stoul(argv[2]);

        int semIDs[2];
        // create semaphores: semIDs[0] initialized with value numPassengers + numPlanes and semIDs[1] initialized with value 0

        semIDs[0] = semget(IPC_PRIVATE, 1, IPC_CREAT | 0666);
        semIDs[1] = semget(IPC_PRIVATE, 1, IPC_CREAT | 0666);

        if (semIDs[0] == -1 || semIDs[1] == -1)
        {
                perror("semget");
                exit(1);
        }

        semctl(semIDs[0], 0, SETVAL, numPassengers + numPlanes);
        semctl(semIDs[1], 0, SETVAL, 0);

        initPassengers(numPassengers, pids[0], semIDs);
        initPlanes(numPlanes, pids[0], semIDs);

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

        std::cout << "Main process: " << getpid() << " waiting for passengers and planes to init\n";
        sembuf buf = {0, 0, 0};
        if (semop(semIDs[0], &buf, 1) == -1)
        {
                perror("semop");
                exit(1);
        }
        std::cout << "Main process: " << getpid() << " released\n";

        // increment semaphore 1 by 1 numPassengers + numPlanes times
        for (size_t i = 0; i < numPassengers + numPlanes; ++i)
        {
                sembuf buf = {0, 1, 0};
                if (semop(semIDs[1], &buf, 1) == -1)
                {
                        perror("semop");
                        exit(1);
                }
        }

        // TODO: runtime


        // testing
        if (getpid() == pids[0])
                std::cout << "Main process: " << getpid() << std::endl;
        else if (getpid() == pids[1])
                std::cout << "SecControl process: " << getpid() << std::endl;
        else if (getpid() == pids[2])
                std::cout << "Dispatcher process: " << getpid() << std::endl;

        // cleanup
        if (semctl(semIDs[0], 0, IPC_RMID) == -1)
        {
                perror("semctl");
                exit(1);
        }

        if (semctl(semIDs[1], 0, IPC_RMID) == -1)
        {
                perror("semctl");
                exit(1);
        }

        return 0;
}
