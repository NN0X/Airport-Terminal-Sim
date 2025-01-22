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

sembuf INC_SEM = {0, 1, 0};
sembuf DEC_SEM = {0, -1, 0};

std::vector<int> initSemaphores(int permissions, size_t numPassengers)
{
        std::vector<int> ids(SEM_NUMBER);

        std::vector<unsigned int> values(ids.size(), SEM_INIT_VALUE);

        for (size_t i = 0; i < ids.size() - 1; i++)
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
        ids[SEM_SECURITY_CONTROL_SELECTOR] = semget(IPC_PRIVATE, numPassengers, permissions);
        if (ids[SEM_SECURITY_CONTROL_SELECTOR] == -1)
        {
                perror("semget");
                exit(1);
        }

        std::vector<uint16_t> initial(numPassengers, SEM_INIT_VALUE);

        if (semctl(ids[SEM_SECURITY_CONTROL_SELECTOR], 0, SETALL, initial.data()) == -1)
        {
                perror("semctl");
                exit(1);
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

void genRandom(uint64_t &val, uint64_t min, uint64_t max)
{
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<uint64_t> dis(min, max);

        val = dis(gen);
}

void vCout(const std::string &msg, int color)
{
        if (VERBOSE == 0)
                return;
        std::cout << colors[color];
        std::cout << msg;
        std::cout << colors[COLOR_NONE];
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
                if (getpid() == pids[PROCESS_MAIN])
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

void createSubprocess(pid_t &pid, const std::string &name)
{
        pid = fork();
        if (pid == -1)
        {
                perror("fork");
                exit(1);
        }
        if (pid != 0)
        {
                if (name != "" && prctl(PR_SET_NAME, name.c_str()) == -1)
                {
                        perror("prctl");
                        exit(1);
                }
                vCout("Created process: " + std::to_string(pid) + "\n");
                vCout("Process name: " + name + "\n");
        }
}
