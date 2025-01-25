#ifndef ARGS_H
#define ARGS_H

#include <sys/types.h>

struct PassengerProcessArgs
{
        size_t id;
        size_t pid;
        pid_t pidMain;
        pid_t pidDispatcher;
        pid_t pidStairs;
        pid_t pidSecurityControl;
        int semIDBaggageControlEntrance;
        int semIDBaggageControlOut;
        int semIDSecurityControlEntrance;
        int semIDSecurityControlEntranceWait;
        int semIDSecurityControlSelector;
        int semIDSecurityControlSelectorEntranceWait;
        int semIDSecurityControlSelectorWait;
        int semIDSecurityGates[3];
        int semIDSecurityGatesWait[3];
        int semIDSecurityControlOut;
        int semIDStairsPassengerIn;
        int semIDStairsPassengerWait;
        int semIDPlanePassengerIn;
        int semIDPlanePassengerWait;
        int semIDPassengerCounter;
};

struct PlaneProcessArgs
{
        size_t id;
        size_t pid;
        int semIDPlaneWait;
        int semIDPlanePassengerIn;
        int semIDPlanePassengerWait;
        int semIDPlaneDepart;
        pid_t pidStairs;
        pid_t pidDispatcher;
        int semIDStairsCounter;
        int semIDPlaneCounter;
};

struct BaggageControlArgs
{
        int semIDBaggageControlEntrance;
        int semIDBaggageControlOut;
};

struct SecurityControlArgs
{
        int semIDSecurityControlEntrance;
        int semIDSecurityControlEntranceWait;
        int semIDSecurityControlSelector;
        int semIDSecurityControlSelectorEntranceWait;
        int semIDSecurityControlSelectorWait;
        int semIDSecurityGate0;
        int semIDSecurityGate1;
        int semIDSecurityGate2;
        int semIDSecurityGate0Wait;
        int semIDSecurityGate1Wait;
        int semIDSecurityGate2Wait;
        int semIDSecurityControlOut;
};

struct SecuritySelectorArgs
{
        int semIDSecuritySelector;
};

struct SecurityGateArgs
{
        int semIDSecurityGate;
        int semIDSecurityGateWait;
        int semIDSecurityControlOut;
};

struct StairsArgs
{
        int semIDStairsWait;
        int semIDStairsPassengerIn;
        int semIDStairsPassengerWait;
        int semIDStairsCounter;
        int semIDPlaneCounter;
};

struct DispatcherArgs
{
        pid_t pidMain;
        int semIDPlaneWait;
        int semIDPlaneDepart;
        int semIDStairsWait;
        int semIDPassengerCounter;
};

#endif // ARGS_H
