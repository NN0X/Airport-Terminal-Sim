#include <iostream>
#include <random>
#include <cstdint>
#include <climits>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h> // O_RDONLY, O_WRONLY
#include <sys/stat.h> // mkfifo
#include <signal.h>
#include <sys/sem.h>

#include "utils.h"
#include "passenger.h"

static bool isOK = false;
static pid_t dispatcherPID;

void passengerSignalHandler(int signum)
{
        switch (signum)
        {
                case SIGNAL_OK:
                        syncedCout("Passenger " + std::to_string(getpid()) + ": Received signal OK\n");
                        isOK = true;
                        break;
                case SIGNAL_PASSENGER_IS_OVERWEIGHT:
                        syncedCout("Passenger " + std::to_string(getpid()) + ": Received signal PASSENGER_IS_OVERWEIGHT\n");
                        kill(dispatcherPID, SIGNAL_PASSENGER_LEFT);
                        exit(0);
                case SIGNAL_PASSENGER_IS_DANGEROUS:
                        syncedCout("Passenger " + std::to_string(getpid()) + ": Received signal PASSENGER_IS_DANGEROUS\n");
                        kill(dispatcherPID, SIGNAL_PASSENGER_LEFT);
                        exit(0);
                default:
                        syncedCout("Passenger " + std::to_string(getpid()) + ": Received unknown signal\n");
                        break;
        }
}


void passengerProcess(size_t id, pid_t pidDispatcher, int semIDBaggageCtrl, int semIDSecCtrl, std::vector<int> semIDGates, int semIDStairs1, int semIDStairs2, int semIDPlane1, int semIDPlane2)
{
        syncedCout("Passenger process: " + std::to_string(id) + "\n");

        dispatcherPID = pidDispatcher;
        Passenger passenger(id);

        // set signal handler
        struct sigaction sa;
        sa.sa_handler = passengerSignalHandler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;
        if (sigaction(SIGNAL_OK, &sa, NULL) == -1)
        {
                perror("sigaction");
                exit(1);
        }
        if (sigaction(SIGNAL_PASSENGER_IS_OVERWEIGHT, &sa, NULL) == -1)
        {
                perror("sigaction");
                exit(1);
        }

        // TODO: run passenger tasks here

        // INFO: passenger goes through baggage control

        // wait for baggage control to be ready
        syncedCout("Passenger: " + std::to_string(getpid()) + " waiting for baggage control\n");
        sembuf dec = {0, -1, 0};
        if (semop(semIDBaggageCtrl, &dec, 1) == -1)
        {
                perror("semop");
                exit(1);
        }

        BaggageInfo baggageInfo;
        baggageInfo.mPid = getpid();
        baggageInfo.mBaggageWeight = passenger.getBaggageWeight();
        int fd = open(fifoNames[BAGGAGE_CONTROL_FIFO].c_str(), O_WRONLY);
        if (fd == -1)
        {
                perror("open");
                exit(1);
        }
        if (write(fd, &baggageInfo, sizeof(baggageInfo)) == -1)
        {
                perror("write");
                exit(1);
        }
        close(fd);

        syncedCout("Passenger: " + std::to_string(getpid()) + " waiting for signal from baggage control\n");
        pause(); // wait for signal from baggage control

        syncedCout("Passenger: " + std::to_string(getpid()) + " released from baggage control\n");

        // INFO: passenger goes through security control

        // wait for security control to be ready
        syncedCout("Passenger: " + std::to_string(getpid()) + " waiting for security control\n");
        if (semop(semIDSecCtrl, &dec, 1) == -1)
        {
                perror("semop");
                exit(1);
        }

        TypeInfo typeInfo;
        typeInfo.mPid = getpid();
        typeInfo.mType = passenger.getType();
        typeInfo.mIsVip = passenger.getIsVip();
        fd = open(fifoNames[SEC_CONTROL_FIFO].c_str(), O_WRONLY);
        if (fd == -1)
        {
                perror("open");
                exit(1);
        }
        if (write(fd, &typeInfo, sizeof(typeInfo)) == -1)
        {
                perror("write");
                exit(1);
        }
        close(fd);

        syncedCout("Passenger: " + std::to_string(getpid()) + " waiting for signal from security control\n");
        pause(); // wait for signal from security control

        syncedCout("Passenger: " + std::to_string(getpid()) + " waiting for security selector\n");
        SelectedPair selectedPair;
        fd = open(fifoNames[SEC_SELECTOR_FIFO].c_str(), O_RDONLY);
        if (fd == -1)
        {
                perror("open");
                exit(1);
        }
        if (read(fd, &selectedPair.gate, sizeof(selectedPair.gate)) == -1)
        {
                perror("read");
                exit(1);
        }
        close(fd);

        syncedCout("Passenger: " + std::to_string(getpid()) + " selected gate: " + std::to_string(selectedPair.gate) + "\n");
        syncedCout("Passenger: " + std::to_string(getpid()) + " waiting for gate " + std::to_string(selectedPair.gate) + "\n");

        if (semop(semIDGates[selectedPair.gate], &dec, 1) == -1)
        {
                perror("semop");
                exit(1);
        }

        DangerInfo dangerInfo;
        dangerInfo.mPid = getpid();
        dangerInfo.mHasDangerousBaggage = passenger.getHasDangerousBaggage();
        fd = open(fifoNames[SEC_GATE_0_FIFO + selectedPair.gate].c_str(), O_WRONLY);
        if (fd == -1)
        {
                perror("open");
                exit(1);
        }
        if (write(fd, &dangerInfo, sizeof(dangerInfo)) == -1)
        {
                perror("write");
                exit(1);
        }
        close(fd);

        while (!isOK)
        {
                usleep(100); // wait for signal from gate
        }

        // INFO: passenger waits for plane to be ready

        syncedCout("Passenger: " + std::to_string(getpid()) + " waiting at stairs\n");
        sembuf inc = {0, 1, 0};
        while (semop(semIDStairs1, &inc, 1) == -1)
        {
                if (errno == EINTR)
                {
                        continue;
                }
                perror("semop");
                exit(1);
        }
        while (semop(semIDStairs2, &dec, 1) == -1)
        {
                if (errno == EINTR)
                {
                        continue;
                }
                perror("semop");
                exit(1);
        }

        syncedCout("Passenger: " + std::to_string(getpid()) + " entering plane\n");
        while (semop(semIDPlane1, &inc, 1) == -1)
        {
                if (errno == EINTR)
                {
                        continue;
                }
                perror("semop");
                exit(1);
        }
        while (semop(semIDPlane2, &dec, 1) == -1)
        {
                if (errno == EINTR)
                {
                        continue;
                }
                perror("semop");
                exit(1);
        }
        syncedCout("Passenger: " + std::to_string(getpid()) + " entered plane\n");

        kill(pidDispatcher, SIGNAL_PASSENGER_LEFT);

        exit(0);
}

void spawnPassengers(size_t num, const std::vector<uint64_t> &delays, pid_t pidDispatcher, int semIDBaggageCtrl, int semIDSecCtrl, std::vector<int> semIDGates, int semIDStairs1, int semIDStairs2, int semIDPlane1, int semIDPlane2)
{
        pid_t pid = getpid();
        std::vector<pid_t> pids(1);
        pids[0] = pid;
        syncedCout("Spawn passengers\n");
        for (size_t i = 0; i < num; i++)
        {
                createSubprocesses(1, pids, {"passenger"});
                if (getpid() != pid)
                {
                        passengerProcess(i, pidDispatcher, semIDBaggageCtrl, semIDSecCtrl, semIDGates, semIDStairs1, semIDStairs2, semIDPlane1, semIDPlane2);
                        exit(0);
                }
                syncedCout("Waiting for " + std::to_string(delays[i]) + " ms\n");
                usleep(delays[i] * 1000);
        }

        syncedCout("All passengers created\n");

        exit(0);
}

Passenger::Passenger(uint64_t id)
        : mID(id), mIsAggressive(false)
{
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<uint64_t> dis(0, UINT64_MAX);

        // vip == true when random number < VIP_PROBABILITY * UINT64_MAX
        mIsVip = dis(gen) < VIP_PROBABILITY * UINT64_MAX;
        mType = dis(gen) < 0.5 * UINT64_MAX;
        mBaggageWeight = dis(gen) % MAX_BAGGAGE_WEIGHT;
        mHasDangerousBaggage = dis(gen) < DANGEROUS_BAGGAGE_PROBABILITY * UINT64_MAX;

        vCout("Passenger " + std::to_string(mID) + " created with the following properties:\n");
        vCout("VIP: " + std::to_string(mIsVip) + "\n");
        vCout("Type: " + std::to_string(mType) + "\n");
        vCout("Baggage weight: " + std::to_string(mBaggageWeight) + "\n");
        vCout("Aggressive: " + std::to_string(mIsAggressive) + "\n");
        vCout("Dangerous baggage: " + std::to_string(mHasDangerousBaggage) + "\n");
}

Passenger::Passenger(uint64_t id, bool isVip, bool type, uint64_t baggageWeight, bool hasDangerousBaggage)
        : mID(id), mIsVip(isVip), mType(type), mBaggageWeight(baggageWeight), mIsAggressive(false), mHasDangerousBaggage(hasDangerousBaggage)
{
        vCout("Passenger " + std::to_string(mID) + " created with the following properties:\n");
        vCout("VIP: " + std::to_string(mIsVip) + "\n");
        vCout("Type: " + std::to_string(mType) + "\n");
        vCout("Baggage weight: " + std::to_string(mBaggageWeight) + "\n");
        vCout("Aggressive: " + std::to_string(mIsAggressive) + "\n");
        vCout("Dangerous baggage: " + std::to_string(mHasDangerousBaggage) + "\n");
}

uint64_t Passenger::getID() const
{
        return mID;
}

bool Passenger::getIsVip() const
{
        return mIsVip;
}

bool Passenger::getType() const
{
        return mType;
}

uint64_t Passenger::getBaggageWeight() const
{
        return mBaggageWeight;
}

bool Passenger::getIsAggressive() const
{
        return mIsAggressive;
}

bool Passenger::getHasDangerousBaggage() const
{
        return mHasDangerousBaggage;
}
