#include <iostream>
#include <sys/types.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <errno.h>
#include <pthread.h>
#include <fcntl.h> // O_RDONLY, O_WRONLY
#include <sys/stat.h> // mkfifo
#include <unistd.h>
#include <array>
#include <vector>
#include <mutex>
#include <signal.h>

#include "passenger.h"
#include "utils.h"

struct GateInfo
{
        int gate;
        int occupancy;
        bool type;
};

static std::mutex secControlMutex;

static std::vector<TypeInfo> secControlQueue;
static std::array<GateInfo, 3> secControlGates = {
        GateInfo{0, 0, false},
        GateInfo{1, 0, false},
        GateInfo{2, 0, false},
};

extern sembuf INC_SEM;
extern sembuf DEC_SEM;

void secControlSignalHandler(int signum)
{
        switch (signum)
        {
                case SIGNAL_OK:
                        vCout("Security control: Received signal OK\n");
                        break;
                case SIGTERM:
                        vCout("Security control: Received signal SIGTERM\n");
                        exit(0);
                default:
                        vCout("Security control: Received unknown signal\n");
                        break;
        }
}

SelectedPair selectVIPPair()
{
        secControlMutex.lock();
        if (secControlQueue.empty())
        {
                secControlMutex.unlock();
                return SelectedPair{-1, -1};
        }
        std::vector<int> vips;
        std::vector<bool> checked;
        for (size_t i = 0; i < secControlQueue.size(); i++)
        {
                if (secControlQueue[i].isVIP)
                {
                        vips.push_back(i);
                        checked.push_back(false);
                }
        }

        for (size_t i = 0; i < vips.size(); i++)
        {
                if (checked[i])
                {
                        continue;
                }
                for (size_t j = 0; j < secControlGates.size(); j++)
                {
                        if (secControlGates[j].occupancy == 0)
                        {
                                secControlGates[j].occupancy++;
                                secControlGates[j].type = secControlQueue[vips[i]].type;
                                secControlMutex.unlock();
                                return SelectedPair{vips[i], (int)j};
                        }
                        else if (secControlGates[j].type == secControlQueue[vips[i]].type && secControlGates[j].occupancy == 1)
                        {
                                secControlGates[j].occupancy++;
                                secControlMutex.unlock();
                                return SelectedPair{vips[i], (int)j};
                        }
                }
        }

        secControlMutex.unlock();
        return SelectedPair{-1, -1};
}

SelectedPair selectPair()
{
        secControlMutex.lock();
        if (secControlQueue.empty())
        {
                secControlMutex.unlock();
                return SelectedPair{-1, -1};
        }
        std::vector<int> nonVips;
        std::vector<bool> checked;
        for (size_t i = 0; i < secControlQueue.size(); i++)
        {
                if (!secControlQueue[i].isVIP)
                {
                        nonVips.push_back(i);
                        checked.push_back(false);
                }
        }
        for (size_t i = 0; i < nonVips.size(); i++)
        {
                if (checked[i])
                {
                        continue;
                }
                for (size_t j = 0; j < secControlGates.size(); j++)
                {
                        if (secControlGates[j].occupancy == 0)
                        {
                                secControlGates[j].occupancy++;
                                secControlGates[j].type = secControlQueue[nonVips[i]].type;
                                secControlMutex.unlock();
                                return SelectedPair{nonVips[i], (int)j};
                        }
                        else if (secControlGates[j].type == secControlQueue[nonVips[i]].type && secControlGates[j].occupancy == 1)
                        {
                                secControlGates[j].occupancy++;
                                secControlMutex.unlock();
                                return SelectedPair{nonVips[i], (int)j};
                        }
                }
        }

        secControlMutex.unlock();
        return SelectedPair{-1, -1};
}

void signalSkipped(int selected)
{
        secControlMutex.lock();
        for (size_t i = 0; i < selected; i++)
        {
                vCout("Security control: Passenger " + std::to_string(secControlQueue[i].id) + " skipped\n");
                kill(secControlQueue[i].pid, SIGNAL_PASSENGER_SKIPPED);
        }
        secControlMutex.unlock();
}

void *gateSelectorThread(void *args)
{
        int *selectorArgs = (int *)args;
        int semIDSecuritySelector = selectorArgs[0];
        int semIDSecuritySelectorEntranceWait = selectorArgs[1];
        int semIDSecuritySelectorWait = selectorArgs[2];

        while (true)
        {
                SelectedPair selectedPair = selectVIPPair();
                if (selectedPair.passengerIndex == -1)
                {
                        selectedPair = selectPair();
                }
                if (selectedPair.passengerIndex == -1)
                {
                        usleep(SECURITY_SELECTOR_DELAY * 1000);
                        continue;
                }
                vCout("Security selector: Selected passenger " + std::to_string(secControlQueue[selectedPair.passengerIndex].id) + " for gate " + std::to_string(selectedPair.gateIndex) + "\n");
                signalSkipped(selectedPair.passengerIndex);

                // signal selected passenger
                secControlMutex.lock();
                TypeInfo typeInfo = secControlQueue[selectedPair.passengerIndex];
                secControlQueue.erase(secControlQueue.begin() + selectedPair.passengerIndex);
                secControlMutex.unlock();

                vCout("Security selector: informing passenger\n");
                // increment semaphore typeInfo.id in semIDSecurityControlSelector
                sembuf incNthSemaphore = {(uint16_t)typeInfo.id, 1, 0};
                safeSemop(semIDSecuritySelector, &incNthSemaphore, 1);

                safeSemop(semIDSecuritySelectorEntranceWait, &DEC_SEM, 1);

                vCout("Security selector: Waiting for passenger to read from fifo\n");

                int fd;
                safeFIFOOpen(fd, fifoNames[FIFO_SECURITY_SELECTOR], O_WRONLY);
                safeFIFOWrite(fd, &selectedPair.gateIndex, sizeof(selectedPair.gateIndex));

                safeSemop(semIDSecuritySelectorWait, &INC_SEM, 1);

                close(fd);

                vCout("Security selector: waiting " + std::to_string(SECURITY_SELECTOR_DELAY) + " ms\n");
                usleep(SECURITY_SELECTOR_DELAY * 1000);
        }
}

void gateThreadTasks(int gate, SecurityGateArgs args)
{
        DangerInfo dangerInfo;
        int fifo = FIFO_SECURITY_GATE_0 + gate;

        while (true)
        {
                // tell passenger to enter

                safeSemop(args.semIDSecurityGate, &INC_SEM, 1);
                vCout("Security gate " + std::to_string(gate) + ": Waiting for passenger\n");
                int fd;
                safeFIFOOpen(fd, fifoNames[fifo], O_RDONLY);

                safeSemop(args.semIDSecurityGateWait, &DEC_SEM, 1);

                safeFIFORead(fd, &dangerInfo, sizeof(dangerInfo));
                close(fd);

                // send signal to passenger
                if (dangerInfo.hasDangerousBaggage)
                {
                        vCout("Security gate " + std::to_string(gate) + ": Passenger " + std::to_string(dangerInfo.pid) + " has dangerous baggage\n");
                        kill(dangerInfo.pid, SIGNAL_PASSENGER_IS_DANGEROUS);
                        safeSemop(args.semIDSecurityControlOut, &INC_SEM, 1);
                }
                else
                {
                        vCout("Security gate " + std::to_string(gate) + ": Passenger " + std::to_string(dangerInfo.pid) + " has no dangerous baggage\n");
                        safeSemop(args.semIDSecurityControlOut, &INC_SEM, 1);
                }

                secControlMutex.lock();
                secControlGates[gate].occupancy--;
                secControlMutex.unlock();

                uint64_t delay;
                genRandom(delay, SECURITY_GATE_MIN_DELAY, SECURITY_GATE_MAX_DELAY);
                vCout("Security gate " + std::to_string(gate) + ": waiting " + std::to_string(delay) + " ms\n");
                usleep(delay * 1000);
        }

}

void *gate0Thread(void *args)
{
        SecurityGateArgs *securityGateArgs = (SecurityGateArgs *)args;
        gateThreadTasks(0, *securityGateArgs);
}

void *gate1Thread(void *args)
{
        SecurityGateArgs *securityGateArgs = (SecurityGateArgs *)args;
        gateThreadTasks(1, *securityGateArgs);
}

void *gate2Thread(void *args)
{
        SecurityGateArgs *securityGateArgs = (SecurityGateArgs *)args;
        gateThreadTasks(2, *securityGateArgs);
}

int secControl(SecurityControlArgs args)
{
        // attach handler to signals
        struct sigaction sa;
        sa.sa_handler = secControlSignalHandler;
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

        // create fifo for communication with passengers
        if (access(fifoNames[FIFO_SECURITY_CONTROL].c_str(), F_OK) == -1)
        {
                if (mkfifo(fifoNames[FIFO_SECURITY_CONTROL].c_str(), 0666) == -1)
                {
                        perror("mkfifo");
                        exit(1);
                }
        }

        if (access(fifoNames[FIFO_SECURITY_SELECTOR].c_str(), F_OK) == -1)
        {
                if (mkfifo(fifoNames[FIFO_SECURITY_SELECTOR].c_str(), 0666) == -1)
                {
                        perror("mkfifo");
                        exit(1);
                }
        }

        if (access(fifoNames[FIFO_SECURITY_GATE_0].c_str(), F_OK) == -1)
        {
                if (mkfifo(fifoNames[FIFO_SECURITY_GATE_0].c_str(), 0666) == -1)
                {
                        perror("mkfifo");
                        exit(1);
                }
        }

        if (access(fifoNames[FIFO_SECURITY_GATE_1].c_str(), F_OK) == -1)
        {
                if (mkfifo(fifoNames[FIFO_SECURITY_GATE_1].c_str(), 0666) == -1)
                {
                        perror("mkfifo");
                        exit(1);
                }
        }

        if (access(fifoNames[FIFO_SECURITY_GATE_2].c_str(), F_OK) == -1)
        {
                if (mkfifo(fifoNames[FIFO_SECURITY_GATE_2].c_str(), 0666) == -1)
                {
                        perror("mkfifo");
                        exit(1);
                }
        }

        // create thread for gate selector
        pthread_t gateSelector;

        int selectorArgs[] = {args.semIDSecurityControlSelector, args.semIDSecurityControlSelectorEntranceWait, args.semIDSecurityControlSelectorWait};

        if (pthread_create(&gateSelector, NULL, gateSelectorThread, (void *)selectorArgs) != 0)
        {
                perror("pthread_create");
                exit(1);
        }

        pthread_t gate0;
        pthread_t gate1;
        pthread_t gate2;
        SecurityGateArgs gateArgs0 = {args.semIDSecurityGate0, args.semIDSecurityGate0Wait, args.semIDSecurityControlOut};
        SecurityGateArgs gateArgs1 = {args.semIDSecurityGate1, args.semIDSecurityGate1Wait, args.semIDSecurityControlOut};
        SecurityGateArgs gateArgs2 = {args.semIDSecurityGate2, args.semIDSecurityGate2Wait, args.semIDSecurityControlOut};

        if (pthread_create(&gate0, NULL, gate0Thread, &gateArgs0) != 0)
        {
                perror("pthread_create");
                exit(1);
        }
        if (pthread_create(&gate1, NULL, gate1Thread, &gateArgs1) != 0)
        {
                perror("pthread_create");
                exit(1);
        }
        if (pthread_create(&gate2, NULL, gate2Thread, &gateArgs2) != 0)
        {
                perror("pthread_create");
                exit(1);
        }

        while (true)
        {
                // receive passengers and add them to gateSelector queue
                // tell passenger to enter
                safeSemop(args.semIDSecurityControlEntrance, &INC_SEM, 1);
                vCout("Security control: waiting for passenger\n");
                int fd;
                safeFIFOOpen(fd, fifoNames[FIFO_SECURITY_CONTROL], O_RDONLY);
                TypeInfo typeInfo;
                safeSemop(args.semIDSecurityControlEntranceWait, &DEC_SEM, 1);

                safeFIFORead(fd, &typeInfo, sizeof(typeInfo));
                close(fd);

                vCout("Security control: Received passenger " + std::to_string(typeInfo.id) + "\n");

                // add passenger to queue
                secControlMutex.lock();
                secControlQueue.push_back(typeInfo);
                secControlMutex.unlock();
        }
}
