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
#include "events.h"
#include "dispatcher.h"
#include "stairs.h"
#include "utils.h"
#include "args.h"

// TODO: send sigterm to all processes not just baggage control

// TODO: change counter from signal to semaphore (PASSENGER_LEFT)
// TODO: change while loop isOK in passenger to semaphore

int totalPassengers;

static std::vector<pid_t> pids(1);



int main(int argc, char* argv[])
{
        if (argc != 3)
        {
                std::cerr << "Usage: " << argv[0] << " <numPassengers> <numPlanes>\n";
                return 1;
        }

        pids[PROCESS_MAIN] = getpid();

        // main + secControl + dispatcher + n passengers + n planes

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
        securityControlArgs.semIDSecurityControlSelector = semIDs[SEM_SECURITY_CONTROL_SELECTOR];
        securityControlArgs.semIDSecurityGate0 = semIDs[SEM_SECURITY_GATE_0];
        securityControlArgs.semIDSecurityGate1 = semIDs[SEM_SECURITY_GATE_1];
        securityControlArgs.semIDSecurityGate2 = semIDs[SEM_SECURITY_GATE_2];
        securityControlArgs.semIDSecurityControlOut = semIDs[SEM_SECURITY_CONTROL_OUT];

        StairsArgs stairsArgs;
        stairsArgs.semIDStairsWait = semIDs[SEM_STAIRS_WAIT];
        stairsArgs.semIDStairsPassengerIn = semIDs[SEM_STAIRS_PASSENGER_IN];
        stairsArgs.semIDStairsPassengerWait = semIDs[SEM_STAIRS_PASSENGER_WAIT];
        stairsArgs.semIDStairsCounter = semIDs[SEM_STAIRS_COUNTER];
        stairsArgs.semIDPlaneCounter = semIDs[SEM_PLANE_COUNTER];

        DispatcherArgs dispatcherArgs;
        dispatcherArgs.semIDPlaneWait = semIDs[SEM_PLANE_WAIT];
        dispatcherArgs.semIDStairsWait = semIDs[SEM_STAIRS_WAIT];
        dispatcherArgs.semIDPassengerCounter = semIDs[SEM_PASSENGER_COUNTER];

        std::vector<std::string> names = {"baggageControl", "secControl", "stairs", "dispatcher"};

        createSubprocesses(4, pids, names);

        pid_t currentPID = getpid();

        if (currentPID == pids[PROCESS_MAIN])
        {
                std::cout << "Main process: " << currentPID << "\n";
                // TODO: runtime


                PassengerProcessArgs passengerArgs;
                passengerArgs.pidDispatcher = pids[PROCESS_DISPATCHER];
                passengerArgs.pidStairs = pids[PROCESS_STAIRS];
                passengerArgs.pidSecurityControl = pids[PROCESS_SECURITY_CONTROL];
                passengerArgs.semIDBaggageControlEntrance = semIDs[SEM_BAGGAGE_CONTROL_ENTRANCE];
                passengerArgs.semIDBaggageControlOut = semIDs[SEM_BAGGAGE_CONTROL_OUT];
                passengerArgs.semIDSecurityControlEntrance = semIDs[SEM_SECURITY_CONTROL_ENTRANCE];
                passengerArgs.semIDSecurityControlSelector = semIDs[SEM_SECURITY_CONTROL_SELECTOR];
                passengerArgs.semIDSecurityGates[0] = semIDs[SEM_SECURITY_GATE_0];
                passengerArgs.semIDSecurityGates[1] = semIDs[SEM_SECURITY_GATE_1];
                passengerArgs.semIDSecurityGates[2] = semIDs[SEM_SECURITY_GATE_2];
                passengerArgs.semIDSecurityControlOut = semIDs[SEM_SECURITY_CONTROL_OUT];
                passengerArgs.semIDStairsPassengerIn = semIDs[SEM_STAIRS_PASSENGER_IN];
                passengerArgs.semIDStairsPassengerWait = semIDs[SEM_STAIRS_PASSENGER_WAIT];
                passengerArgs.semIDPlanePassengerIn = semIDs[SEM_PLANE_PASSANGER_IN];
                passengerArgs.semIDPlanePassengerWait = semIDs[SEM_PLANE_PASSANGER_WAIT];
                passengerArgs.semIDPassengerCounter = semIDs[SEM_PASSENGER_COUNTER];

                PlaneProcessArgs planeArgs;
                planeArgs.semIDPlaneWait = semIDs[SEM_PLANE_WAIT];
                planeArgs.semIDPlanePassengerIn = semIDs[SEM_PLANE_PASSANGER_IN];
                planeArgs.semIDPlanePassengerWait = semIDs[SEM_PLANE_PASSANGER_WAIT];
                planeArgs.pidStairs = pids[PROCESS_STAIRS];
                planeArgs.pidDispatcher = pids[PROCESS_DISPATCHER];
                planeArgs.semIDStairsCounter = semIDs[SEM_STAIRS_COUNTER];
                planeArgs.semIDPlaneCounter = semIDs[SEM_PLANE_COUNTER];

                std::vector<uint64_t> delays(numPassengers);
                genRandomVector(delays, 0, MAX_PASSENGER_DELAY);

                initPlanes(numPlanes, planeArgs);

                createSubprocesses(1, pids, {"spawnPassengers"});
                if (getpid() != currentPID)
                {
                        spawnPassengers(numPassengers, delays, passengerArgs);
                }

                while (true)
                {
                        // INFO: wait for signal to exit
                        if (getc(stdin) == 'q')
                        {
                                for (size_t i = 1; i < pids.size(); i++)
                                {
                                        kill(pids[i], SIGTERM);
                                }
                                break;
                        }
                }
                // INFO: cleanup

                // delete semaphores
                for (size_t i = 0; i < semIDs.size(); i++)
                {
                        if (semctl(semIDs[i], 0, IPC_RMID) == -1)
                        {
                                perror("semctl");
                                exit(1);
                        }
                }
                // delete fifo
                if (unlink("baggageControlFIFO") == -1)
                {
                        perror("unlink");
                        exit(1);
                }
                vCout("Main process: Exiting\n");
        }
        else if (currentPID == pids[PROCESS_BAGGAGE_CONTROL])
        {
                std::cout << "Baggage control process: " << getpid() << "\n";
                baggageControl(baggageControlArgs);
        }
        else if (currentPID == pids[PROCESS_SECURITY_CONTROL])
        {
                std::cout << "Security control process: " << getpid() << "\n";
                secControl(securityControlArgs);
        }
        else if (currentPID == pids[PROCESS_STAIRS])
        {
                std::cout << "Stairs process: " << getpid() << "\n";
                stairs(stairsArgs);
        }
        else if (currentPID == pids[PROCESS_DISPATCHER])
        {
                std::cout << "Dispatcher process: " << getpid() << "\n";
                dispatcher(dispatcherArgs);
        }
        else
        {
                std::cerr << "Unknown process\n";
        }

        return 0;
}


