#include <iostream>
#include <vector>
#include <cstdint>
#include <unistd.h>
#include <sys/types.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <sys/prctl.h>
#include <errno.h>
#include <pthread.h>
#include <mutex>
#include <random>
#include <string>
#include <fcntl.h> // O_RDONLY, O_WRONLY
#include <sys/stat.h> // mkfifo
#include <signal.h>

#include "plane.h"
#include "passenger.h"
#include "baggageControl.h"
#include "secControl.h"
#include "dispatcher.h"
#include "stairs.h"
#include "utils.h"
#include "args.h"

int totalPassengers;

static std::vector<pid_t> pids(1);
static std::vector<pid_t> planePIDs;

static bool mainLoop = true;

void mainSignalHandler(int signum)
{
        switch (signum)
        {
        case SIGNAL_OK:
                vCout("Main: Received signal OK\n", NONE, LOG_MAIN);
                break;
        case SIGNAL_THIS_IS_THE_END_HOLD_YOUR_BREATH_AND_COUNT_TO_TEN:
                for (size_t i = 0; i < pids.size(); i++)
                {
                        kill(pids[i], SIGTERM);
                }
                for (size_t i = 0; i < planePIDs.size(); i++)
                {
                        kill(planePIDs[i], SIGTERM);
                }
                mainLoop = false;
                break;
        case SIGTERM:
                vCout("Main: Received signal SIGTERM\n", NONE, LOG_MAIN);
                break;
        default:
                vCout("Main: Received unknown signal\n", NONE, LOG_MAIN);
                break;
        }
}

int main(int argc, char* argv[])
{
        if (argc != 3)
        {
                std::cerr << "Usage: " << argv[0] << " <numPassengers> <numPlanes>\n";
                return 1;
        }

        struct sigaction sa;
        sa.sa_handler = mainSignalHandler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;
        if (sigaction(SIGNAL_OK, &sa, NULL) == -1)
        {
                perror("sigaction");
                exit(1);
        }
        if (sigaction(SIGNAL_THIS_IS_THE_END_HOLD_YOUR_BREATH_AND_COUNT_TO_TEN, &sa, NULL) == -1)
        {
                perror("sigaction");
                exit(1);
        }
        if (sigaction(SIGTERM, &sa, NULL) == -1)
        {
                perror("sigaction");
                exit(1);
        }

        pids[PROCESS_MAIN] = getpid();

        size_t numPassengers = std::stoul(argv[1]);
        size_t numPlanes = std::stoul(argv[2]);

        totalPassengers = numPassengers;

        if (numPassengers > 30000 || numPlanes > 30000)
        {
                std::cerr << "Maximum values: <numPassengers> = 30000, <numPlanes> = 30000\n";
                return 1;
        }

        std::vector<int> semIDs = initSemaphores(0666, numPassengers);

        BaggageControlArgs baggageControlArgs;
        baggageControlArgs.semIDBaggageControlEntrance = semIDs[SEM_BAGGAGE_CONTROL_ENTRANCE];
        baggageControlArgs.semIDBaggageControlOut = semIDs[SEM_BAGGAGE_CONTROL_OUT];

        SecurityControlArgs securityControlArgs;
        securityControlArgs.semIDSecurityControlEntrance = semIDs[SEM_SECURITY_CONTROL_ENTRANCE];
        securityControlArgs.semIDSecurityControlEntranceWait = semIDs[SEM_SECURITY_CONTROL_ENTRANCE_WAIT];
        securityControlArgs.semIDSecurityControlSelector = semIDs[SEM_SECURITY_CONTROL_SELECTOR];
        securityControlArgs.semIDSecurityControlSelectorEntranceWait = semIDs[SEM_SECURITY_CONTROL_SELECTOR_ENTRANCE_WAIT];
        securityControlArgs.semIDSecurityControlSelectorWait = semIDs[SEM_SECURITY_CONTROL_SELECTOR_WAIT];
        securityControlArgs.semIDSecurityGate0 = semIDs[SEM_SECURITY_GATE_0];
        securityControlArgs.semIDSecurityGate1 = semIDs[SEM_SECURITY_GATE_1];
        securityControlArgs.semIDSecurityGate2 = semIDs[SEM_SECURITY_GATE_2];
        securityControlArgs.semIDSecurityGate0Wait = semIDs[SEM_SECURITY_GATE_0_WAIT];
        securityControlArgs.semIDSecurityGate1Wait = semIDs[SEM_SECURITY_GATE_1_WAIT];
        securityControlArgs.semIDSecurityGate2Wait = semIDs[SEM_SECURITY_GATE_2_WAIT];
        securityControlArgs.semIDSecurityControlOut = semIDs[SEM_SECURITY_CONTROL_OUT];

        StairsArgs stairsArgs;
        stairsArgs.semIDStairsWait = semIDs[SEM_STAIRS_WAIT];
        stairsArgs.semIDStairsPassengerIn = semIDs[SEM_STAIRS_PASSENGER_IN];
        stairsArgs.semIDStairsPassengerWait = semIDs[SEM_STAIRS_PASSENGER_WAIT];
        stairsArgs.semIDStairsCounter = semIDs[SEM_STAIRS_COUNTER];
        stairsArgs.semIDPlaneCounter = semIDs[SEM_PLANE_COUNTER];

        DispatcherArgs dispatcherArgs;
        dispatcherArgs.semIDPlaneWait = semIDs[SEM_PLANE_WAIT];
        dispatcherArgs.semIDPlaneDepart = semIDs[SEM_PLANE_DEPART];
        dispatcherArgs.semIDStairsWait = semIDs[SEM_STAIRS_WAIT];
        dispatcherArgs.semIDPassengerCounter = semIDs[SEM_PASSENGER_COUNTER];

        std::vector<std::string> names = {"baggageControl", "secControl", "stairs", "dispatcher"};

        createSubprocesses(4, pids, names);

        pid_t currentPID = getpid();

        if (currentPID == pids[PROCESS_MAIN])
        {
                vCout("Main process: " + std::to_string(currentPID) + "\n", NONE, LOG_MAIN);

                PassengerProcessArgs passengerArgs;
                passengerArgs.pidMain = currentPID;
                passengerArgs.pidDispatcher = pids[PROCESS_DISPATCHER];
                passengerArgs.pidStairs = pids[PROCESS_STAIRS];
                passengerArgs.pidSecurityControl = pids[PROCESS_SECURITY_CONTROL];
                passengerArgs.semIDBaggageControlEntrance = semIDs[SEM_BAGGAGE_CONTROL_ENTRANCE];
                passengerArgs.semIDBaggageControlOut = semIDs[SEM_BAGGAGE_CONTROL_OUT];
                passengerArgs.semIDSecurityControlEntrance = semIDs[SEM_SECURITY_CONTROL_ENTRANCE];
                passengerArgs.semIDSecurityControlEntranceWait = semIDs[SEM_SECURITY_CONTROL_ENTRANCE_WAIT];
                passengerArgs.semIDSecurityControlSelector = semIDs[SEM_SECURITY_CONTROL_SELECTOR];
                passengerArgs.semIDSecurityControlSelectorEntranceWait = semIDs[SEM_SECURITY_CONTROL_SELECTOR_ENTRANCE_WAIT];
                passengerArgs.semIDSecurityControlSelectorWait = semIDs[SEM_SECURITY_CONTROL_SELECTOR_WAIT];
                passengerArgs.semIDSecurityGates[0] = semIDs[SEM_SECURITY_GATE_0];
                passengerArgs.semIDSecurityGates[1] = semIDs[SEM_SECURITY_GATE_1];
                passengerArgs.semIDSecurityGates[2] = semIDs[SEM_SECURITY_GATE_2];
                passengerArgs.semIDSecurityGatesWait[0] = semIDs[SEM_SECURITY_GATE_0_WAIT];
                passengerArgs.semIDSecurityGatesWait[1] = semIDs[SEM_SECURITY_GATE_1_WAIT];
                passengerArgs.semIDSecurityGatesWait[2] = semIDs[SEM_SECURITY_GATE_2_WAIT];
                passengerArgs.semIDSecurityControlOut = semIDs[SEM_SECURITY_CONTROL_OUT];
                passengerArgs.semIDStairsPassengerIn = semIDs[SEM_STAIRS_PASSENGER_IN];
                passengerArgs.semIDStairsPassengerWait = semIDs[SEM_STAIRS_PASSENGER_WAIT];
                passengerArgs.semIDPlanePassengerIn = semIDs[SEM_PLANE_PASSANGER_IN];
                passengerArgs.semIDPlanePassengerWait = semIDs[SEM_PLANE_PASSANGER_WAIT];
                passengerArgs.semIDPassengerCounter = semIDs[SEM_PASSENGER_COUNTER];

                PlaneProcessArgs planeArgs;
                planeArgs.semIDPlaneWait = semIDs[SEM_PLANE_WAIT];
                planeArgs.semIDPlaneDepart = semIDs[SEM_PLANE_DEPART];
                planeArgs.semIDPlanePassengerIn = semIDs[SEM_PLANE_PASSANGER_IN];
                planeArgs.semIDPlanePassengerWait = semIDs[SEM_PLANE_PASSANGER_WAIT];
                planeArgs.pidStairs = pids[PROCESS_STAIRS];
                planeArgs.pidDispatcher = pids[PROCESS_DISPATCHER];
                planeArgs.semIDStairsCounter = semIDs[SEM_STAIRS_COUNTER];
                planeArgs.semIDPlaneCounter = semIDs[SEM_PLANE_COUNTER];

                std::vector<uint64_t> delays(numPassengers);
                genRandomVector(delays, 0, MAX_PASSENGER_DELAY);

                planePIDs = initPlanes(numPlanes, planeArgs);

                createSubprocesses(1, pids, {"spawnPassengers"});
                if (getpid() != currentPID)
                {
                        spawnPassengers(numPassengers, delays, passengerArgs);
                }

                while (mainLoop)
                {
                        char c = getc(stdin);
                        if (c == 'q')
                        {
                                kill(0, SIGNAL_THIS_IS_THE_END_HOLD_YOUR_BREATH_AND_COUNT_TO_TEN);
                        }
                        else if (c == 'p')
                        {
                                kill(pids[PROCESS_DISPATCHER], SIGNAL_DISPATCHER_PLANE_FORCED_DEPART);
                                vCout("Main process: Forced plane depart\n", NONE, LOG_MAIN);
                        }
                }

                for (size_t i = 0; i < semIDs.size(); i++)
                {
                        semctl(semIDs[i], 0, IPC_RMID);
                }

                for (size_t i = 0; i < FIFO_SECURITY_GATE_2 + 1; i++)
                {
                        unlink(fifoNames[i].c_str());
                }

                vCout("Main process: Exiting\n", NONE, LOG_MAIN);
        }
        else if (currentPID == pids[PROCESS_BAGGAGE_CONTROL])
        {
                vCout("Baggage control: " + std::to_string(getpid()) + "\n", NONE, LOG_MAIN);
                baggageControl(baggageControlArgs);
        }
        else if (currentPID == pids[PROCESS_SECURITY_CONTROL])
        {
                vCout("Security control process: " + std::to_string(getpid()) + "\n", NONE, LOG_MAIN);
                secControl(securityControlArgs);
        }
        else if (currentPID == pids[PROCESS_STAIRS])
        {
                vCout("Stairs process: " + std::to_string(getpid()) + "\n", NONE, LOG_MAIN);
                stairs(stairsArgs);
        }
        else if (currentPID == pids[PROCESS_DISPATCHER])
        {
                vCout("Dispatcher process: " + std::to_string(getpid()) + "\n", NONE, LOG_MAIN);
                dispatcher(dispatcherArgs);
        }
        else
        {
                vCout("Unknown process\n", NONE, LOG_MAIN);
        }

        return 0;
}


