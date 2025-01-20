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

#define SEC_SELECTOR_DELAY 100 // in ms
#define SEC_GATE_MAX_DELAY 500 // in ms
#define SEC_GATE_MIN_DELAY 200 // in ms

struct GateInfo
{
        int gate;
        int occupancy;
        bool type;
};

std::mutex secControlToGateSelectorMutex;
std::mutex secControlToGateMutex;

std::vector<TypeInfo> secControlQueue;
std::array<GateInfo, 3> secControlGates = {
        GateInfo{0, 0, false},
        GateInfo{1, 0, false},
        GateInfo{2, 0, false},
};

struct SelectorArgs
{
        int semID;
};

struct GateArgs
{
        int semID;
};

void secControlSignalHandler(int signum)
{
        switch (signum)
        {
                case SIGNAL_OK:
                        syncedCout("Security control: Received signal OK\n");
                        break;
                case SIGTERM:
                        syncedCout("Security control: Received signal SIGTERM\n");
                        exit(0);
                default:
                        syncedCout("Security control: Received unknown signal\n");
                        break;
        }
}

SelectedPair selectVIPPair()
{
        secControlToGateSelectorMutex.lock();
        secControlToGateMutex.lock();
        if (secControlQueue.empty())
        {
                secControlToGateMutex.unlock();
                secControlToGateSelectorMutex.unlock();
                return SelectedPair{-1, -1};
        }
        std::vector<int> vips;
        std::vector<bool> checked;
        for (size_t i = 0; i < secControlQueue.size(); i++)
        {
                if (secControlQueue[i].mIsVip)
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
                                secControlGates[j].type = secControlQueue[vips[i]].mType;
                                secControlToGateMutex.unlock();
                                secControlToGateSelectorMutex.unlock();
                                return SelectedPair{vips[i], (int)j};
                        }
                        else if (secControlGates[j].type == secControlQueue[vips[i]].mType && secControlGates[j].occupancy == 1)
                        {
                                secControlGates[j].occupancy++;
                                secControlToGateMutex.unlock();
                                secControlToGateSelectorMutex.unlock();
                                return SelectedPair{vips[i], (int)j};
                        }
                }
        }

        secControlToGateMutex.unlock();
        secControlToGateSelectorMutex.unlock();
        return SelectedPair{-1, -1};
}

SelectedPair selectPair()
{
        secControlToGateSelectorMutex.lock();
        secControlToGateMutex.lock();
        if (secControlQueue.empty())
        {
                secControlToGateMutex.unlock();
                secControlToGateSelectorMutex.unlock();
                return SelectedPair{-1, -1};
        }
        std::vector<int> nonVips;
        std::vector<bool> checked;
        for (size_t i = 0; i < secControlQueue.size(); i++)
        {
                if (!secControlQueue[i].mIsVip)
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
                                secControlGates[j].type = secControlQueue[nonVips[i]].mType;
                                secControlToGateMutex.unlock();
                                secControlToGateSelectorMutex.unlock();
                                return SelectedPair{nonVips[i], (int)j};
                        }
                        else if (secControlGates[j].type == secControlQueue[nonVips[i]].mType && secControlGates[j].occupancy == 1)
                        {
                                secControlGates[j].occupancy++;
                                secControlToGateMutex.unlock();
                                secControlToGateSelectorMutex.unlock();
                                return SelectedPair{nonVips[i], (int)j};
                        }
                }
        }

        secControlToGateMutex.unlock();
        secControlToGateSelectorMutex.unlock();
        return SelectedPair{-1, -1};
}

void signalSkipped(int selected)
{
        secControlToGateSelectorMutex.lock();
        for (size_t i = 0; i < selected; i++)
        {
                syncedCout("Security control: Passenger " + std::to_string(secControlQueue[i].mPid) + " skipped\n");
                // send signal to passenger
        }
        secControlToGateSelectorMutex.unlock();
}

void *gateSelectorThread(void *args)
{
        while (true)
        {
                // select passenger from queue and send him to gate
                // rules of selection:
                // 1. vip passengers have priority
                // 2. each gate can have 2 same type passengers at the same time and can't have 2 passengers of different types
                // 3. if passenger is skipped because of rule 2, signal him
                // 4. if passenger is allowed to enter, send him the gate number (there are 3 gates)
                // 5. repeat until signal to exit

                SelectedPair selectedPair = selectVIPPair();
                if (selectedPair.passenger == -1)
                {
                        selectedPair = selectPair();
                }
                if (selectedPair.passenger == -1)
                {
                        usleep(SEC_SELECTOR_DELAY * 1000);
                        continue;
                }
                syncedCout("Security selector: Selected passenger " + std::to_string(secControlQueue[selectedPair.passenger].mPid) + " for gate " + std::to_string(selectedPair.gate) + "\n");
                signalSkipped(selectedPair.passenger);

                // signal selected passenger
                secControlToGateSelectorMutex.lock();
                TypeInfo typeInfo = secControlQueue[selectedPair.passenger];
                secControlQueue.erase(secControlQueue.begin() + selectedPair.passenger);
                secControlToGateSelectorMutex.unlock();

                syncedCout("Security selector: Sending signal OK to passenger\n");
                // send signal SIGNAL_OK to passenger
                kill(typeInfo.mPid, SIGNAL_OK);

                syncedCout("Security selector: Waiting for passenger to read from fifo\n");
                int fd = open(fifoNames[SEC_SELECTOR_FIFO].c_str(), O_WRONLY);
                if (fd == -1)
                {
                        perror("open");
                        exit(1);
                }
                if (write(fd, &selectedPair.gate, sizeof(selectedPair.gate)) == -1)
                {
                        perror("write");
                        exit(1);
                }
                close(fd);

                syncedCout("Security selector: waiting " + std::to_string(SEC_SELECTOR_DELAY) + " ms\n");
                usleep(SEC_SELECTOR_DELAY * 1000);
        }
}

void gateThreadTasks(int gate, int semID)
{
        DangerInfo dangerInfo;
        int fifo;
        switch (gate)
        {
                case 0:
                        fifo = SEC_GATE_0_FIFO;
                        break;
                case 1:
                        fifo = SEC_GATE_1_FIFO;
                        break;
                case 2:
                        fifo = SEC_GATE_2_FIFO;
                        break;
                default:
                        return;
        }

        while (true)
        {
                // tell passenger to be enter
                sembuf inc = {0, 1, 0};
                if (semop(semID, &inc, 1) == -1)
                {
                        perror("semop");
                        exit(1);
                }
                syncedCout("Security gate " + std::to_string(gate) + ": Waiting for passenger\n");
                int fd = open(fifoNames[fifo].c_str(), O_RDONLY);
                if (fd == -1)
                {
                        perror("open");
                        exit(1);
                }
                if (read(fd, &dangerInfo, sizeof(dangerInfo)) == -1)
                {
                        perror("read");
                        exit(1);
                }
                close(fd);

                // send signal to passenger
                if (dangerInfo.mHasDangerousBaggage)
                {
                        syncedCout("Security gate " + std::to_string(gate) + ": Passenger " + std::to_string(dangerInfo.mPid) + " has dangerous baggage\n");
                        kill(dangerInfo.mPid, PASSENGER_IS_DANGEROUS);
                }
                else
                {
                        syncedCout("Security gate " + std::to_string(gate) + ": Passenger " + std::to_string(dangerInfo.mPid) + " has no dangerous baggage\n");
                        kill(dangerInfo.mPid, SIGNAL_OK);
                }
                secControlToGateMutex.lock();
                secControlGates[gate].occupancy--;
                secControlToGateMutex.unlock();

                std::vector<uint64_t> delay(1);
                genRandomVector(delay, SEC_GATE_MIN_DELAY, SEC_GATE_MAX_DELAY);
                syncedCout("Security gate " + std::to_string(gate) + ": waiting " + std::to_string(delay[0]) + " ms\n");
                usleep(delay[0] * 1000);
        }

}

void *gate0Thread(void *args)
{
        GateArgs *gateArgs = (GateArgs *)args;
        gateThreadTasks(0, gateArgs->semID);
}

void *gate1Thread(void *args)
{
        GateArgs *gateArgs = (GateArgs *)args;
        gateThreadTasks(1, gateArgs->semID);
}

void *gate2Thread(void *args)
{
        GateArgs *gateArgs = (GateArgs *)args;
        gateThreadTasks(2, gateArgs->semID);
}

int secControl(int semIDSecControl, int semIDGate0, int semIDGate1, int semIDGate2)
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
        if (access(fifoNames[SEC_CONTROL_FIFO].c_str(), F_OK) == -1)
        {
                if (mkfifo(fifoNames[SEC_CONTROL_FIFO].c_str(), 0666) == -1)
                {
                        perror("mkfifo");
                        exit(1);
                }
        }

        if (access(fifoNames[SEC_SELECTOR_FIFO].c_str(), F_OK) == -1)
        {
                if (mkfifo(fifoNames[SEC_SELECTOR_FIFO].c_str(), 0666) == -1)
                {
                        perror("mkfifo");
                        exit(1);
                }
        }

        if (access(fifoNames[SEC_GATE_0_FIFO].c_str(), F_OK) == -1)
        {
                if (mkfifo(fifoNames[SEC_GATE_0_FIFO].c_str(), 0666) == -1)
                {
                        perror("mkfifo");
                        exit(1);
                }
        }

        if (access(fifoNames[SEC_GATE_1_FIFO].c_str(), F_OK) == -1)
        {
                if (mkfifo(fifoNames[SEC_GATE_1_FIFO].c_str(), 0666) == -1)
                {
                        perror("mkfifo");
                        exit(1);
                }
        }

        if (access(fifoNames[SEC_GATE_2_FIFO].c_str(), F_OK) == -1)
        {
                if (mkfifo(fifoNames[SEC_GATE_2_FIFO].c_str(), 0666) == -1)
                {
                        perror("mkfifo");
                        exit(1);
                }
        }

        // create thread for gate selector
        pthread_t gateSelector;

        if (pthread_create(&gateSelector, NULL, gateSelectorThread, NULL) != 0)
        {
                perror("pthread_create");
                exit(1);
        }

        pthread_t gate0;
        pthread_t gate1;
        pthread_t gate2;
        GateArgs gateArgs0 = {semIDGate0};
        GateArgs gateArgs1 = {semIDGate1};
        GateArgs gateArgs2 = {semIDGate2};

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
                sembuf inc = {0, 1, 0};
                if (semop(semIDSecControl, &inc, 1) == -1)
                {
                        perror("semop");
                        exit(1);
                }
                syncedCout("Security control: waiting for passenger\n");
                int fd = open(fifoNames[SEC_CONTROL_FIFO].c_str(), O_RDONLY);
                if (fd == -1)
                {
                        perror("open");
                        exit(1);
                }
                TypeInfo typeInfo;
                if (read(fd, &typeInfo, sizeof(typeInfo)) == -1)
                {
                        perror("read");
                        exit(1);
                }
                close(fd);

                syncedCout("Security control: Received passenger " + std::to_string(typeInfo.mPid) + "\n");

                // add passenger to queue
                secControlToGateSelectorMutex.lock();
                secControlQueue.push_back(typeInfo);
                secControlToGateSelectorMutex.unlock();
        }
}
