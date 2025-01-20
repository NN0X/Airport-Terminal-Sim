#ifndef DISPATCHER_H
#define DISPATCHER_H

#include <vector>
#include <cstdint>
#include <unistd.h>
#include <sys/types.h>

int dispatcher(std::vector<pid_t> planePids, pid_t stairsPid);

#endif // DISPATCHER_H
