#include <iostream>
#include <vector>
#include <cstdint>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <mutex>
#include <signal.h>

#include "utils.h"

void dispatcherSignalHandler(int signum)
{
        switch (signum)
        {
        case SIGNAL_OK:
                syncedCout("Dispatcher: Received signal OK\n");
                break;
        case SIGNAL_PLANE_READY:
                syncedCout("Dispatcher: Received signal PLANE_READY\n");
                break;
        case SIGNAL_PLANE_READY_DEPART:
                syncedCout("Dispatcher: Received signal PLANE_READY_DEPART\n");
                break;
        case SIGTERM:
                syncedCout("Dispatcher: Received signal SIGTERM\n");
                exit(0);
                break;
        default:
                syncedCout("Dispatcher: Received unknown signal\n");
                break;
        }
}

int dispatcher(std::vector<pid_t> planePids, pid_t stairsPid)
{
        // attach handler to signals
        struct sigaction sa;
        sa.sa_handler = dispatcherSignalHandler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;
        if (sigaction(SIGNAL_OK, &sa, NULL) == -1)
        {
                perror("sigaction");
                exit(1);
        }
        if (sigaction(SIGNAL_PLANE_READY, &sa, NULL) == -1)
        {
                perror("sigaction");
                exit(1);
        }
        if (sigaction(SIGNAL_PLANE_READY_DEPART, &sa, NULL) == -1)
        {
                perror("sigaction");
                exit(1);
        }
        if (sigaction(SIGTERM, &sa, NULL) == -1)
        {
                perror("sigaction");
                exit(1);
        }

        int currentPlane = 0;
        int readyPlanes = planePids.size();
        while (true)
        {
                // INFO: signal plane to go to terminal
                // INFO: signal plane to wait for passengers if queue is not empty
                // INFO: signal gate to let passengers board
                // INFO: signal plane when passengers are on board (on stairs)
                // INFO: repeat until signal to exit

                pause(); // wait for signal PLANE_READY
                kill(planePids[currentPlane], SIGNAL_PLANE_RECEIVE);
                kill(stairsPid, SIGNAL_STAIRS_OPEN);

                pause(); // wait for signal PLANE_READY_DEPART
                kill(planePids[currentPlane], SIGNAL_PLANE_GO);
                currentPlane = (currentPlane + 1) % readyPlanes;
        }
}
