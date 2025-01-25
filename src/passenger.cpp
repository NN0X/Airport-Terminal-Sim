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
#include <mutex>

#include "utils.h"
#include "passenger.h"

extern sembuf INC_SEM;
extern sembuf DEC_SEM;

static bool failedBaggageControl = false;
static bool failedSecurityControl = false;

static int skipped = 0;

static pid_t mainPID;

void passengerSignalHandler(int signum)
{
        uint64_t val;
        uint64_t getsAggresiveToInt;
        switch (signum)
        {
                case SIGNAL_OK:
                        vCout("Passenger " + std::to_string(getpid()) + ": Received signal OK\n", GREEN, LOG_PASSENGER);
                        break;
                case SIGNAL_PASSENGER_IS_OVERWEIGHT:
                        vCout("Passenger " + std::to_string(getpid()) + ": Received signal PASSENGER_IS_OVERWEIGHT\n", GREEN, LOG_PASSENGER);
                        getsAggresiveToInt = PASSENGER_GETS_AGGRESSIVE_BAGGAGE_PROBABILITY * UINT64_MAX;
                        genRandom(val, 0, UINT64_MAX);
                        if (val < getsAggresiveToInt)
                        {
                                vCout("Passenger " + std::to_string(getpid()) + ": Passenger gets aggressive\n", GREEN, LOG_PASSENGER);
                                kill(mainPID, SIGNAL_THIS_IS_THE_END_HOLD_YOUR_BREATH_AND_COUNT_TO_TEN);
                        }
                        failedBaggageControl = true;
                        break;
                case SIGNAL_PASSENGER_IS_DANGEROUS:
                        vCout("Passenger " + std::to_string(getpid()) + ": Received signal PASSENGER_IS_DANGEROUS\n", GREEN, LOG_PASSENGER);
                        getsAggresiveToInt = PASSENGER_GETS_AGGRESSIVE_SECURITY_PROBABILITY * UINT64_MAX;
                        genRandom(val, 0, UINT64_MAX);
                        if (val < getsAggresiveToInt)
                        {
                                vCout("Passenger " + std::to_string(getpid()) + ": Passenger gets aggressive\n", GREEN, LOG_PASSENGER);
                                kill(mainPID, SIGNAL_THIS_IS_THE_END_HOLD_YOUR_BREATH_AND_COUNT_TO_TEN);
                        }
                        failedSecurityControl = true;
                        break;
                case SIGNAL_PASSENGER_SKIPPED:
                        vCout("Passenger " + std::to_string(getpid()) + ": Received signal PASSENGER_SKIPPED\n", GREEN, LOG_PASSENGER);
                        skipped++;
                        if (skipped == PASSENGER_MAX_SKIPPED && WORST_CASE_SCENARIO)
                        {
                                vCout("Passenger " + std::to_string(getpid()) + ": Skipped 3 times\n", GREEN, LOG_PASSENGER);
                                kill(mainPID, SIGNAL_THIS_IS_THE_END_HOLD_YOUR_BREATH_AND_COUNT_TO_TEN);
                        }
                        else if (skipped == PASSENGER_MAX_SKIPPED && !WORST_CASE_SCENARIO)
                        {
                                vCout("Passenger " + std::to_string(getpid()) + ": Skipped 3 times\n", GREEN, LOG_PASSENGER);
                                exit(0);
                        }
                        break;
                case SIGTERM:
                        vCout("Passenger " + std::to_string(getpid()) + ": Received signal SIGTERM\n", GREEN, LOG_PASSENGER);
                        exit(0);
                default:
                        vCout("Passenger " + std::to_string(getpid()) + ": Received unknown signal\n", GREEN, LOG_PASSENGER);
                        break;
        }
}

static int semIDPassengerCounterGlobal;

void atExitPassenger()
{
        vCout("Passenger: " + std::to_string(getpid()) + " exiting\n", GREEN, LOG_PASSENGER);
        safeSemop(semIDPassengerCounterGlobal, &INC_SEM, 1);
}

void passengerProcess(PassengerProcessArgs args)
{
        vCout("Passenger process: " + std::to_string(args.id) + "\n", GREEN, LOG_PASSENGER);

        Passenger passenger(args.id);
        semIDPassengerCounterGlobal = args.semIDPassengerCounter;
        mainPID = args.pidMain;

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
        vCout("Passenger: " + std::to_string(args.id) + " waiting for baggage control\n", GREEN, LOG_PASSENGER);

        safeSemop(args.semIDBaggageControlEntrance, &DEC_SEM, 1);

        BaggageInfo baggageInfo;
        baggageInfo.pid = args.pid;
        baggageInfo.weight = passenger.getBaggageWeight();
        int fd;
        safeFIFOOpen(fd, fifoNames[FIFO_BAGGAGE_CONTROL], O_WRONLY);
        safeFIFOWrite(fd, &baggageInfo, sizeof(baggageInfo));

        vCout("Passenger: " + std::to_string(args.id) + " waiting for signal from baggage control\n", GREEN, LOG_PASSENGER);

        safeSemop(args.semIDBaggageControlOut, &DEC_SEM, 1);

        if (failedBaggageControl)
        {
                vCout("Passenger: " + std::to_string(args.id) + " failed baggage control\n", GREEN, LOG_PASSENGER);
                exit(0);
        }

        vCout("Passenger: " + std::to_string(args.id) + " released from baggage control\n", GREEN, LOG_PASSENGER);

        // INFO: passenger goes through security control

        vCout("Passenger: " + std::to_string(args.id) + " waiting for security control\n", GREEN, LOG_PASSENGER);

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

        vCout("Passenger: " + std::to_string(args.id) + " waiting for security control selector\n", GREEN, LOG_PASSENGER);

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

        vCout("Passenger: " + std::to_string(args.id) + " selected gate: " + std::to_string(selectedPair.gateIndex) + "\n", GREEN, LOG_PASSENGER);
        vCout("Passenger: " + std::to_string(args.id) + " waiting for gate " + std::to_string(selectedPair.gateIndex) + "\n", GREEN, LOG_PASSENGER);

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
                vCout("Passenger: " + std::to_string(args.id) + " failed security control\n", GREEN, LOG_PASSENGER);
                exit(0);
        }

        // INFO: passenger waits for plane to be ready

        safeSemop(args.semIDStairsPassengerIn, &INC_SEM, 1);

        vCout("Passenger: " + std::to_string(args.id) + " waiting at stairs\n", GREEN, LOG_PASSENGER);

        safeSemop(args.semIDStairsPassengerWait, &DEC_SEM, 1);

        vCout("Passenger: " + std::to_string(args.id) + " entering plane\n", GREEN, LOG_PASSENGER);

        safeSemop(args.semIDPlanePassengerIn, &INC_SEM, 1);

        safeSemop(args.semIDPlanePassengerWait, &DEC_SEM, 1);

        vCout("Passenger: " + std::to_string(args.id) + " entered plane\n", GREEN, LOG_PASSENGER);

        exit(0);
}

static std::vector<pid_t> passengerPIDs;

static std::mutex spawnPassengersMutex;

void spawnPassengersSignalHandler(int signum)
{
        switch (signum)
        {
                case SIGNAL_OK:
                        vCout("Spawn passengers: Received signal OK\n", NONE, LOG_MAIN);
                        break;
                case SIGTERM:
                        vCout("Spawn passengers: Received signal SIGTERM\n", NONE, LOG_MAIN);
                        spawnPassengersMutex.lock();
                        for (size_t i = 0; i < passengerPIDs.size(); i++)
                        {
                                kill(passengerPIDs[i], SIGTERM);
                        }
                        spawnPassengersMutex.unlock();
                        exit(0);
                        break;
                default:
                        vCout("Spawn passengers: Received unknown signal\n", NONE, LOG_MAIN);
                        break;
        }
}

void spawnPassengers(size_t num, const std::vector<uint64_t> &delays, PassengerProcessArgs args)
{
        struct sigaction sa;
        sa.sa_handler = spawnPassengersSignalHandler;
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

        pid_t oldPID = getpid();
        vCout("Spawn passengers\n", NONE, LOG_MAIN);
        for (size_t i = 0; i < num; i++)
        {
                pid_t newPID;
                createSubprocess(newPID, "passenger");
                spawnPassengersMutex.lock();
                passengerPIDs.push_back(newPID);
                spawnPassengersMutex.unlock();
                if (getpid() != oldPID)
                {
                        args.id = i;
                        args.pid = getpid();
                        passengerProcess(args);
                        exit(0);
                }
                vCout("Spawn passengers: Waiting for " + std::to_string(delays[i]) + " ms\n", NONE, LOG_MAIN);
                usleep(delays[i] * 1000);
        }

        vCout("All passengers created\n", NONE, LOG_MAIN);
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

        vCout("Passenger " + std::to_string(mID) + " created with the following properties:\n", GREEN, LOG_PASSENGER);
        vCout("VIP: " + std::to_string(mIsVip) + "\n", GREEN, LOG_PASSENGER);
        vCout("Type: " + std::to_string(mType) + "\n", GREEN, LOG_PASSENGER);
        vCout("Baggage weight: " + std::to_string(mBaggageWeight) + "\n", GREEN, LOG_PASSENGER);
        vCout("Aggressive: " + std::to_string(mIsAggressive) + "\n", GREEN, LOG_PASSENGER);
        vCout("Dangerous baggage: " + std::to_string(mHasDangerousBaggage) + "\n", GREEN, LOG_PASSENGER);
}

Passenger::Passenger(uint64_t id, bool isVip, bool type, uint64_t baggageWeight, bool hasDangerousBaggage)
        : mID(id), mIsVip(isVip), mType(type), mBaggageWeight(baggageWeight), mIsAggressive(false), mHasDangerousBaggage(hasDangerousBaggage)
{
        vCout("Passenger " + std::to_string(mID) + " created with the following properties:\n", GREEN, LOG_PASSENGER);
        vCout("VIP: " + std::to_string(mIsVip) + "\n", GREEN, LOG_PASSENGER);
        vCout("Type: " + std::to_string(mType) + "\n", GREEN, LOG_PASSENGER);
        vCout("Baggage weight: " + std::to_string(mBaggageWeight) + "\n", GREEN, LOG_PASSENGER);
        vCout("Aggressive: " + std::to_string(mIsAggressive) + "\n", GREEN, LOG_PASSENGER);
        vCout("Dangerous baggage: " + std::to_string(mHasDangerousBaggage) + "\n", GREEN, LOG_PASSENGER);
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
