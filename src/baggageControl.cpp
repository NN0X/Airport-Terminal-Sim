#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h> // O_RDONLY, O_WRONLY
#include <sys/stat.h> // mkfifo
#include <unistd.h>
#include <sys/sem.h>

#include "passenger.h"
#include "utils.h"

void baggageControlSignalHandler(int signum)
{
        switch (signum)
        {
                case SIGNAL_OK:
                        syncedCout("Baggage control: Received signal OK\n");
                        break;
                case SIGTERM:
                        syncedCout("Baggage control: Received signal SIGTERM\n");
                        exit(0);
                default:
                        syncedCout("Baggage control: Received unknown signal\n");
                        break;
        }
}

int baggageControl(int semID)
{
        // INFO: check if passenger baggage weight is within limits
        // INFO: if not, signal event handler
        // INFO: repeat until signal to exit

        // check if fifo exists
        if (access(fifoNames[BAGGAGE_CONTROL_FIFO].c_str(), F_OK) == -1)
        {
                if (mkfifo(fifoNames[BAGGAGE_CONTROL_FIFO].c_str(), 0666) == -1)
                {
                        perror("mkfifo");
                        exit(1);
                }
        }

        // set signal handler
        struct sigaction sa;
        sa.sa_handler = baggageControlSignalHandler;
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

        while (true)
        {
                // increment semaphore
                syncedCout("Baggage control: incrementing semaphore\n");
                sembuf inc = {0, 1, 0};
                if (semop(semID, &inc, 1) == -1)
                {
                        perror("semop");
                        exit(1);
                }
                int fd = open(fifoNames[BAGGAGE_CONTROL_FIFO].c_str(), O_RDONLY);
                if (fd == -1)
                {
                        perror("open");
                        exit(1);
                }

                // read from fifo the passenger pid and baggage weight
                BaggageInfo baggageInfo;
                if (read(fd, &baggageInfo, sizeof(baggageInfo)) == -1)
                {
                        perror("read");
                        exit(1);
                }
                close(fd);
                syncedCout("Baggage control: received baggage info\n");

                if (baggageInfo.mBaggageWeight > MAX_ALLOWED_BAGGAGE_WEIGHT)
                {
                        syncedCout("Baggage control: passenger " + std::to_string(baggageInfo.mPid) + " is overweight\n");
                        kill(baggageInfo.mPid, PASSENGER_IS_OVERWEIGHT);
                        // TODO: consider sending PASSENGER_IS_OVERWEIGHT signal to event handler
                        // and then event handler sending signal to passenger
                }
                else
                {
                        syncedCout("Baggage control: passenger " + std::to_string(baggageInfo.mPid) + " is not overweight\n");
                        // send empty signal to unblock passenger
                        kill(baggageInfo.mPid, SIGNAL_OK);
                }
        }
}
