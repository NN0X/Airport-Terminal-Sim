#include <iostream>
#include <vector>
#include <cstdint>
#include <unistd.h>
#include <sys/types.h>
#include <sys/prctl.h>
#include <errno.h>
#include <mutex>
#include <string>
#include <random>
#include <sys/sem.h>

#include "utils.h"

std::mutex coutMutex;

#define VERBOSE 1
#define SEM_INIT_VALUE 0

#define NUM_SEMAPHORES 9

std::vector<int> initSemaphores(int permissions)
{
        std::vector<int> ids(NUM_SEMAPHORES);

        std::vector<unsigned int> values(ids.size(), SEM_INIT_VALUE);

        for (size_t i = 0; i < ids.size(); i++)
        {
                ids[i] = semget(IPC_PRIVATE, 1, permissions);
                if (ids[i] == -1)
                {
                        perror("semget");
                        exit(1);
                }
                if (semctl(ids[i], 0, SETVAL, values[i]) == -1)
                {
                        perror("semctl");
                        exit(1);
                }
        }

        return ids;
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

void vCout(const std::string &msg, int color)
{
        if (VERBOSE == 0)
                return;
        std::cout << colors[color];
        std::cout << msg;
        std::cout << colors[NONE];
}

void syncedCout(const std::string &msg, int color)
{
        //std::lock_guard<std::mutex> lock(coutMutex);
        vCout(msg, color);
}

void createSubprocesses(size_t n, std::vector<pid_t> &pids, const std::vector<std::string> &names)
{
        size_t offset = pids.size();
        pids.resize(pids.size() + n);
        if (names.size() != n)
        {
                std::cerr << "Number of names does not match number of processes\n";
                std::cerr << "Names: " << names.size() << ", Processes: " << n << "\n";
                exit(1);
        }

        for (size_t i = offset; i < n + offset; i++)
        {
                pids[i] = fork();
                if (getpid() == pids[MAIN])
                {
                        continue;
                }
                if (pids[i] == -1)
                {
                        perror("fork");
                        exit(1);
                }
                pids[i] = getpid();
                if (pids[i] != 0)
                {
                        if (names[i - offset] != "" && prctl(PR_SET_NAME, names[i - offset].c_str()) == -1)
                        {
                                perror("prctl");
                                exit(1);
                        }
                        vCout("Created process: " + std::to_string(pids[i]) + "\n");
                        vCout("Process name: " + names[i - offset] + "\n");
                        return;
                }
        }
}

