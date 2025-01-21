#ifndef STAIRS_H
#define STAIRS_H

#include <cstdint>
#include <sys/types.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <errno.h>
#include <mutex>
#include <signal.h>

int stairs(int semID1, int semID2, int semStairsCounter, int semPlaneCounter);

#endif // STAIRS_H
