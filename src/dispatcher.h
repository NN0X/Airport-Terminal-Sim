#ifndef DISPATCHER_H
#define DISPATCHER_H

#include <vector>
#include <cstdint>
#include <unistd.h>
#include <sys/types.h>

#include "args.h"

int dispatcher(DispatcherArgs args);

#endif // DISPATCHER_H
