#include <iostream>
#include <sys/types.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <errno.h>
#include <pthread.h>
#include <fcntl.h> // O_RDONLY, O_WRONLY
#include <sys/stat.h> // mkfifo
#include <unistd.h>

#include "passenger.h"
#include "utils.h"

#define SEC_RECEIVER_DELAY 100 // in ms
#define SEC_GATE_MAX_DELAY 500 // in ms
#define SEC_GATE_MIN_DELAY 200 // in ms

void secControlSignalHandler(int signum)
{
        switch (signum)
        {
                case SIGNAL_OK:
                        syncedCout("Security control: Received signal OK\n");
                        break;
                case SEC_CONTROL_PASSENGER_WAITING:
                        syncedCout("Security control: Received signal PASSENGER_WAITING\n");
                        break;
                case SIGTERM:
                        syncedCout("Security control: Received signal SIGTERM\n");
                        exit(0);
                default:
                        syncedCout("Security control: Received unknown signal\n");
                        break;
        }
}

struct GateInfo
{
        int num;
        int occupancy;
        bool type;
        int semID1;
        int semID2;
};

void *secGateThread(void *arg)
{
        GateInfo *gateInfo = (GateInfo *)arg;

        syncedCout("Gate " + std::to_string(gateInfo->num) + " thread\n");

        if (access(fifoNames[SEC_GATE_FIFO + gateInfo->num].c_str(), F_OK) == -1)
        {
                if (mkfifo(fifoNames[SEC_GATE_FIFO + gateInfo->num].c_str(), 0666) == -1)
                {
                        perror("mkfifo");
                        exit(1);
                }
        }

        while (true)
        {
                syncedCout("Gate " + std::to_string(gateInfo->num) + ": waiting for passenger\n");
                // increment semaphore
                sembuf inc = {0, 1, 0};
                if (semop(gateInfo->semID1, &inc, 1) == -1)
                {
                        perror("semop");
                        exit(1);
                }

                sembuf dec = {0, -1, 0};
                if (semop(gateInfo->semID2, &dec, 1) == -1)
                {
                        perror("semop");
                        exit(1);
                }

                syncedCout("Gate " + std::to_string(gateInfo->num) + ": passenger entered\n");

                int fd = open(fifoNames[SEC_GATE_FIFO + gateInfo->num].c_str(), O_RDONLY);
                if (fd == -1)
                {
                        perror("open");
                        exit(1);
                }
                BaggageDangerInfo baggageDangerInfo;
                if (read(fd, &baggageDangerInfo, sizeof(baggageDangerInfo)) == -1)
                {
                        perror("read");
                        exit(1);
                }
                close(fd);
                if (baggageDangerInfo.mHasDangerousBaggage)
                {
                        syncedCout("Gate " + std::to_string(gateInfo->num) + ": Passenger " + std::to_string(baggageDangerInfo.mPid) + " has dangerous baggage\n");
                        kill(baggageDangerInfo.mPid, PASSENGER_IS_DANGEROUS);
                        gateInfo->occupancy--;
                }
                else
                {
                        syncedCout("Gate " + std::to_string(gateInfo->num) + ": Passenger " + std::to_string(baggageDangerInfo.mPid) + " has no dangerous baggage\n");
                        kill(baggageDangerInfo.mPid, SIGNAL_OK);
                        gateInfo->occupancy--;
                }
                std::vector<uint64_t> delays(1);
                genRandomVector(delays, SEC_GATE_MIN_DELAY, SEC_GATE_MAX_DELAY);
                syncedCout("Waiting for " + std::to_string(delays[0]) + " ms\n", BLUE);
                usleep(delays[0] * 1000);
        }

}

struct ReceivePassengerArgs
{
        std::vector<TypeInfo> *typeInfos;
        int semID;
        int semIDReceive;
};

void *receivePassengerThread(void *arg)
{
        syncedCout("Receive passengers thread\n");

        if (access(fifoNames[SEC_RECEIVE_FIFO].c_str(), F_OK) == -1)
        {
                if (mkfifo(fifoNames[SEC_RECEIVE_FIFO].c_str(), 0666) == -1)
                {
                        perror("mkfifo");
                        exit(1);
                }
        }

        ReceivePassengerArgs *args = (ReceivePassengerArgs *)arg;
        std::vector<TypeInfo> &typeInfos = *args->typeInfos;

        int semID = args->semID;
        int semIDReceive = args->semIDReceive;

        int fd = open(fifoNames[SEC_RECEIVE_FIFO].c_str(), O_RDONLY);
        if (fd == -1)
        {
                perror("open");
                exit(1);
        }

        while (true)
        {
                syncedCout("Receive passengers: waiting for passengers\n");

                // decrement semaphore
                sembuf dec = {0, -1, 0};
                if (semop(semIDReceive, &dec, 1) == -1)
                {
                        perror("semop");
                        exit(1);
                }
                syncedCout("Receive passengers: passenger entered security\n");

                TypeInfo typeInfo;
                if (read(fd, &typeInfo, sizeof(typeInfo)) == -1)
                {
                        perror("read");
                        exit(1);
                }
                // increment semaphore
                typeInfos.push_back(typeInfo);
                sembuf inc = {0, 1, 0};
                if (semop(semID, &inc, 1) == -1)
                {
                        perror("semop");
                        exit(1);
                }
                syncedCout("Receive passengers: passenger " + std::to_string(typeInfo.mPid) + " received\n", RED);
                syncedCout("Waiting for " + std::to_string(SEC_RECEIVER_DELAY) + " ms\n", BLUE);
                usleep(SEC_RECEIVER_DELAY * 1000);
        }
        close(fd);
}

PassengerGatePair pairPassengerGate(std::vector<TypeInfo> &typeInfos, const std::vector<GateInfo> &gateInfos)
{
        PassengerGatePair pair;
        for (size_t i = 0; i < 3; i++)
        {
                if (gateInfos[i].occupancy == 0 || (gateInfos[i].type == typeInfos[0].mType && gateInfos[i].occupancy < 2))
                {
                        pair.pid = typeInfos[0].mPid;
                        pair.gateNum = i;
                        typeInfos.erase(typeInfos.begin());
                        syncedCout("Pairing passenger " + std::to_string(pair.pid) + " with gate " + std::to_string(pair.gateNum) + "\n");
                        return pair;
                }
                else if (typeInfos.size() > 1 && (gateInfos[i].occupancy == 0 || (gateInfos[i].type == typeInfos[1].mType && gateInfos[i].occupancy < 2)))
                {
                        pair.pid = typeInfos[1].mPid;
                        pair.gateNum = i;
                        typeInfos.erase(typeInfos.begin() + 1);
                        syncedCout("Pairing passenger " + std::to_string(pair.pid) + " with gate " + std::to_string(pair.gateNum) + "\n");
                        return pair;
                }
        }
        syncedCout("No pairing found\n", RED);
        return pair;
}

int secControl(int semID, int semIDReceive, int *semIDsGate1, int *semIDsGate2, int *semIDsGate3)
{
        syncedCout("Security control process\n");
        if (access(fifoNames[SEC_CONTROL_FIFO].c_str(), F_OK) == -1)
        {
                if (mkfifo(fifoNames[SEC_CONTROL_FIFO].c_str(), 0666) == -1)
                {
                        perror("mkfifo");
                        exit(1);
                }
        }

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

        std::vector<GateInfo> gateInfos(3);
        for (size_t i = 0; i < 3; i++)
        {
                gateInfos[i].num = i;
                gateInfos[i].occupancy = 0;
                gateInfos[i].type = false;
        }
        gateInfos[0].semID1 = semIDsGate1[0];
        gateInfos[1].semID1 = semIDsGate2[0];
        gateInfos[2].semID1 = semIDsGate3[0];
        gateInfos[0].semID2 = semIDsGate1[1];
        gateInfos[1].semID2 = semIDsGate2[1];
        gateInfos[2].semID2 = semIDsGate3[1];
        pthread_t gateThreads[3];
        for (size_t i = 0; i < 3; i++)
        {
                if (pthread_create(&gateThreads[i], NULL, secGateThread, (void *)&gateInfos[i]) != 0)
                {
                        perror("pthread_create");
                        exit(1);
                }
        }

        std::vector<TypeInfo> typeInfos;
        typeInfos.reserve(100);
        ReceivePassengerArgs typeInfosArgs = {&typeInfos, semID, semIDReceive};
        pthread_t receivePassenger;
        if (pthread_create(&receivePassenger, NULL, receivePassengerThread, (void *)&typeInfosArgs) != 0)
        {
                perror("pthread_create");
                exit(1);
        }

        PassengerGatePair passengerGatePair;
        while (true)
        {
                syncedCout("Security control: waiting for passengers\n", GREEN);
                // decrement semaphore
                sembuf dec = {0, -1, 0};
                if (semop(semID, &dec, 1) == -1)
                {
                        perror("semop");
                        exit(1);
                }
                syncedCout("Security control: received passengers\n");
                passengerGatePair = pairPassengerGate(typeInfos, gateInfos);
                if (passengerGatePair.pid == -1)
                {
                        continue;
                }
                if(gateInfos[passengerGatePair.gateNum].occupancy == 0)
                {
                        gateInfos[passengerGatePair.gateNum].type = typeInfos[0].mType;
                }
                gateInfos[passengerGatePair.gateNum].occupancy++;
                // signal passenger to read from fifo
                syncedCout("Security control: signaling passenger " + std::to_string(passengerGatePair.pid) + "\n");
                kill(passengerGatePair.pid, SIGNAL_OK);

                syncedCout("Security control: sending gate number to passenger " + std::to_string(passengerGatePair.pid) + "\n");
                int fd = open(fifoNames[SEC_CONTROL_FIFO].c_str(), O_WRONLY);
                if (fd == -1)
                {
                        perror("open");
                        exit(1);
                }
                if (write(fd, &passengerGatePair, sizeof(passengerGatePair)) == -1)
                {
                        perror("write");
                        exit(1);
                }
                close(fd);
        }
        syncedCout("Security control: Exiting\n");

        for (size_t i = 0; i < 3; i++)
        {
                if (pthread_join(gateThreads[i], NULL) != 0)
                {
                        perror("pthread_join");
                        exit(1);
                }
        }
        if (pthread_join(receivePassenger, NULL) != 0)
        {
                perror("pthread_join");
                exit(1);
        }

        return 0;
}
