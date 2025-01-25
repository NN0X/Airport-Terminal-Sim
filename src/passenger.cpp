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

extern sembuf INC_SEM;
extern sembuf DEC_SEM;

static bool failedBaggageControl = false;
static bool failedSecurityControl = false;

void passengerSignalHandler(int signum)
{
        switch (signum)
        {
                case SIGNAL_OK:
                        vCout("Passenger " + std::to_string(getpid()) + ": Received signal OK\n");
                        break;
                case SIGNAL_PASSENGER_IS_OVERWEIGHT:
                        vCout("Passenger " + std::to_string(getpid()) + ": Received signal PASSENGER_IS_OVERWEIGHT\n");
                        failedBaggageControl = true;
                        break;
                case SIGNAL_PASSENGER_IS_DANGEROUS:
                        vCout("Passenger " + std::to_string(getpid()) + ": Received signal PASSENGER_IS_DANGEROUS\n");
                        failedSecurityControl = true;
                        break;
                case SIGNAL_PASSENGER_SKIPPED:
                        vCout("Passenger " + std::to_string(getpid()) + ": Received signal PASSENGER_SKIPPED\n");
                        break;
                case SIGTERM:
                        vCout("Passenger " + std::to_string(getpid()) + ": Received signal SIGTERM\n");
                        exit(0);
                default:
                        vCout("Passenger " + std::to_string(getpid()) + ": Received unknown signal\n");
                        break;
        }
}

static int semIDPassengerCounterGlobal;

void atExitPassenger()
{
        safeSemop(semIDPassengerCounterGlobal, &INC_SEM, 1);
}

void passengerProcess(PassengerProcessArgs args)
{
        vCout("Passenger process: " + std::to_string(args.id) + "\n");

        Passenger passenger(args.id);
        semIDPassengerCounterGlobal = args.semIDPassengerCounter;

        if (atexit(atExitPassenger) != 0)
        {
                perror("atexit");
                exit(1);
        }

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
        if (sigaction(SIGNAL_PASSENGER_IS_DANGEROUS, &sa, NULL) == -1)
        {
                perror("sigaction");
                exit(1);
        }
        if (sigaction(SIGNAL_PASSENGER_SKIPPED, &sa, NULL) == -1)
        {
                perror("sigaction");
                exit(1);
        }
        if (sigaction(SIGTERM, &sa, NULL) == -1)
        {
                perror("sigaction");
                exit(1);
        }

        // TODO: run passenger tasks here

        // INFO: passenger goes through baggage control

        // wait for baggage control to be ready
        vCout("Passenger: " + std::to_string(args.id) + " waiting for baggage control\n");

        safeSemop(args.semIDBaggageControlEntrance, &DEC_SEM, 1);

        BaggageInfo baggageInfo;
        baggageInfo.pid = args.pid;
        baggageInfo.weight = passenger.getBaggageWeight();
        int fd;
        safeFIFOOpen(fd, fifoNames[FIFO_BAGGAGE_CONTROL], O_WRONLY);
        safeFIFOWrite(fd, &baggageInfo, sizeof(baggageInfo));

        vCout("Passenger: " + std::to_string(args.id) + " waiting for signal from baggage control\n");

        safeSemop(args.semIDBaggageControlOut, &DEC_SEM, 1);

        if (failedBaggageControl)
        {
                vCout("Passenger: " + std::to_string(args.id) + " failed baggage control\n");
                exit(0);
        }

        vCout("Passenger: " + std::to_string(args.id) + " released from baggage control\n");

        // INFO: passenger goes through security control

        vCout("Passenger: " + std::to_string(args.id) + " waiting for security control\n");

        safeSemop(args.semIDSecurityControlEntrance, &DEC_SEM, 1);

        TypeInfo typeInfo;
        typeInfo.id = args.id;
        typeInfo.pid = args.pid;
        typeInfo.type = passenger.getType();
        typeInfo.isVIP = passenger.getIsVip();

        safeFIFOOpen(fd, fifoNames[FIFO_SECURITY_CONTROL], O_WRONLY);
        safeFIFOWrite(fd, &typeInfo, sizeof(typeInfo));

        safeSemop(args.semIDSecurityControlEntranceWait, &INC_SEM, 1);

        close(fd);

        vCout("Passenger: " + std::to_string(args.id) + " waiting for security control selector\n");

        // wait until semIDSecurityControlSelector semaphore number args.id is 1
        sembuf decreaseNthSemaphore = {(uint16_t)args.id, -1, 0};
        safeSemop(args.semIDSecurityControlSelector, &decreaseNthSemaphore, 1);

        safeSemop(args.semIDSecurityControlSelectorEntranceWait, &INC_SEM, 1);

        vCout("Passenger: " + std::to_string(args.id) + " waiting for security selector\n");
        SelectedPair selectedPair;
        safeFIFOOpen(fd, fifoNames[FIFO_SECURITY_SELECTOR], O_RDONLY);

        safeSemop(args.semIDSecurityControlSelectorWait, &DEC_SEM, 1);

        safeFIFORead(fd, &selectedPair.gateIndex, sizeof(selectedPair.gateIndex));
        close(fd);

        vCout("Passenger: " + std::to_string(args.id) + " selected gate: " + std::to_string(selectedPair.gateIndex) + "\n");
        vCout("Passenger: " + std::to_string(args.id) + " waiting for gate " + std::to_string(selectedPair.gateIndex) + "\n");

        safeSemop(args.semIDSecurityGates[selectedPair.gateIndex], &DEC_SEM, 1);

        DangerInfo dangerInfo;
        dangerInfo.pid = args.pid;
        dangerInfo.hasDangerousBaggage = passenger.getHasDangerousBaggage();

        safeFIFOOpen(fd, fifoNames[FIFO_SECURITY_GATE_0 + selectedPair.gateIndex], O_WRONLY);
        safeFIFOWrite(fd, &dangerInfo, sizeof(dangerInfo));

        safeSemop(args.semIDSecurityGatesWait[selectedPair.gateIndex], &INC_SEM, 1);
        close(fd);

        safeSemop(args.semIDSecurityControlOut, &DEC_SEM, 1);

        if (failedSecurityControl)
        {
                vCout("Passenger: " + std::to_string(args.id) + " failed security control\n");
                exit(0);
        }

        // INFO: passenger waits for plane to be ready

        safeSemop(args.semIDStairsPassengerIn, &INC_SEM, 1);

        vCout("Passenger: " + std::to_string(args.id) + " waiting at stairs\n");

        std::cout << "Passenger: " << args.id << " waiting for stairs\n";

        safeSemop(args.semIDStairsPassengerWait, &DEC_SEM, 1);

        vCout("Passenger: " + std::to_string(args.id) + " entering plane\n");

        safeSemop(args.semIDPlanePassengerIn, &INC_SEM, 1);

        std::cout << "Passenger: " << args.id << " waiting for plane\n";

        safeSemop(args.semIDPlanePassengerWait, &DEC_SEM, 1);

        std::cout << "Passenger: " << args.id << " entered plane\n";
        vCout("Passenger: " + std::to_string(args.id) + " entered plane\n");

        kill(args.pidStairs, SIGNAL_PASSENGER_LEFT_STAIRS);

        exit(0);
}

void spawnPassengers(size_t num, const std::vector<uint64_t> &delays, PassengerProcessArgs args)
{
        pid_t oldPID = getpid();
        vCout("Spawn passengers\n");
        for (size_t i = 0; i < num; i++)
        {
                pid_t newPID;
                createSubprocess(newPID, "passenger");
                if (getpid() != oldPID)
                {
                        args.id = i;
                        args.pid = getpid();
                        passengerProcess(args);
                        exit(0);
                }
                vCout("Waiting for " + std::to_string(delays[i]) + " ms\n");
                usleep(delays[i] * 1000);
        }

        vCout("All passengers created\n");

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
