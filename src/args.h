#ifndef ARGS_H
#define ARGS_H

#include <sys/types.h>

struct PassengerProcessArgs
{
        size_t id;
        size_t pid;
        pid_t pidDispatcher;
        pid_t pidStairs;
        pid_t pidSecurityControl;
        int semIDBaggageControlEntrance;
        int semIDBaggageControlOut;
        int semIDSecurityControlEntrance;
        int semIDSecurityControlSelector;
        int semIDSecurityControlSelectorWait;
        int semIDSecurityGates[3];
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
        int semIDSecurityControlSelector;
        int semIDSecurityGate0;
        int semIDSecurityGate1;
        int semIDSecurityGate2;
        int semIDSecurityControlOut;
};

struct SecuritySelectorArgs
{
        int semIDSecuritySelector;
};

struct SecurityGateArgs
{
        int semIDSecurityGate;
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
        int semIDPlaneWait;
        int semIDStairsWait;
        int semIDPassengerCounter;
};

#endif // ARGS_H
