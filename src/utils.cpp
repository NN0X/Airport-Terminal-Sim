#include <iostream>
#include <fstream>
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
#include <fcntl.h> // O_RDONLY, O_WRONLY
#include <sys/stat.h> // mkfifo
#include <signal.h>

#include "utils.h"

sembuf INC_SEM = {0, 1, 0};
sembuf DEC_SEM = {0, -1, 0};

std::vector<int> initSemaphores(int permissions, size_t numPassengers)
{
        std::vector<int> ids(SEM_NUMBER);

        std::vector<unsigned int> values(ids.size(), SEM_INIT_VALUE);

        values[SEM_STAIRS_COUNTER] = STAIRS_MAX_ALLOWED_OCCUPANCY;

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

void appendToLog(const std::string &msg, const std::string &log)
{
        std::ofstream file;
        file.open(log, std::ios_base::app);
        file << msg;
        file.close();
}

void vCout(const std::string &msg, int color, int log)
{
        if (VERBOSE == 0)
                return;
        std::cout << colors[color];
        std::cout << msg;
        std::cout << colors[NONE];
        switch (log)
        {
                case LOG_PASSENGER:
                        appendToLog(msg, "passenger.log");
                        break;
                case LOG_BAGGAGE_CONTROL:
                        appendToLog(msg, "baggageControl.log");
                        break;
                case LOG_SECURITY_CONTROL:
                        appendToLog(msg, "securityControl.log");
                        break;
                case LOG_STAIRS:
                        appendToLog(msg, "stairs.log");
                        break;
                case LOG_DISPATCHER:
                        appendToLog(msg, "dispatcher.log");
                        break;
                case LOG_PLANE:
                        appendToLog(msg, "plane.log");
                        break;
                case LOG_MAIN:
                        appendToLog(msg, "main.log");
                        break;
                default:
                        break;
        }
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

void safeFIFOOpen(int &fd, const std::string &name, int flags)
{
        fd = open(name.c_str(), flags);
        if (fd == -1)
        {
                if (errno == EINTR)
                {
                        safeFIFOOpen(fd, name, flags);
                }
                else
                {
                        perror("open");
                        exit(1);
                }
        }
}

void safeFIFOWrite(int fd, const void *buf, size_t size)
{
        if (write(fd, buf, size) == -1)
        {
                if (errno == EINTR)
                {
                        safeFIFOWrite(fd, buf, size);
                }
                else
                {
                        perror("write");
                        exit(1);
                }
        }
}

void safeFIFORead(int fd, void *buf, size_t size)
{
        if (read(fd, buf, size) == -1)
        {
                if (errno == EINTR)
                {
                        safeFIFORead(fd, buf, size);
                }
                else
                {
                        perror("read");
                        exit(1);
                }
        }
}

void safeFIFOClose(int fd)
{
        if (close(fd) == -1)
        {
                if (errno == EINTR)
                {
                        safeFIFOClose(fd);
                }
                else
                {
                        perror("close");
                        exit(1);
                }
        }
}

void safeSemop(int semID, sembuf *sops, size_t nsops)
{
       if (semop(semID, sops, nsops) == -1)
        {
                if (errno == EINTR)
                {
                        safeSemop(semID, sops, nsops);
                }
                else
                {
                        perror("semop");
                        exit(1);
                }
        }
}

int safeGetSemVal(int semID, int semnum)
{
        int val;
        if ((val = semctl(semID, semnum, GETVAL)) == -1)
        {
                if (errno == EINTR)
                {
                        return safeGetSemVal(semID, semnum);
                }
                else
                {
                        perror("semctl");
                        exit(1);
                }
        }
        return val;
}

void safeSetSemVal(int semID, int semnum, int val)
{
        if (semctl(semID, semnum, SETVAL, val) == -1)
        {
                if (errno == EINTR)
                {
                        safeSetSemVal(semID, semnum, val);
                }
                else
                {
                        perror("semctl");
                        exit(1);
                }
        }
}
