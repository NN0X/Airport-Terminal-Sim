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

extern sembuf INC_SEM;
extern sembuf DEC_SEM;

void baggageControlSignalHandler(int signum)
{
        switch (signum)
        {
                case SIGNAL_OK:
                        vCout("Baggage control: Received signal OK\n", BLUE, LOG_BAGGAGE_CONTROL);
                        break;
                case SIGTERM:
                        vCout("Baggage control: Received signal SIGTERM\n", BLUE, LOG_BAGGAGE_CONTROL);
                        exit(0);
                default:
                        vCout("Baggage control: Received unknown signal\n", BLUE, LOG_BAGGAGE_CONTROL);
                        break;
        }
}

int baggageControl(BaggageControlArgs args)
{
        // INFO: check if passenger baggage weight is within limits
        // INFO: if not, signal event handler
        // INFO: repeat until signal to exit

        // check if fifo exists
        if (access(fifoNames[FIFO_BAGGAGE_CONTROL].c_str(), F_OK) == -1)
        {
                if (mkfifo(fifoNames[FIFO_BAGGAGE_CONTROL].c_str(), 0666) == -1)
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
                // tell passenger to enter
                safeSemop(args.semIDBaggageControlEntrance, &INC_SEM, 1);

                vCout("Baggage control: waiting for passenger\n", BLUE, LOG_BAGGAGE_CONTROL);
                int fd;
                safeFIFOOpen(fd, fifoNames[FIFO_BAGGAGE_CONTROL], O_RDONLY);

                // read from fifo the passenger pid and baggage weight
                BaggageInfo baggageInfo;
                safeFIFORead(fd, &baggageInfo, sizeof(baggageInfo));
                close(fd);
                vCout("Baggage control: received baggage info\n", BLUE, LOG_BAGGAGE_CONTROL);

                if (baggageInfo.weight > PLANE_MAX_ALLOWED_BAGGAGE_WEIGHT)
                {
                        vCout("Baggage control: passenger " + std::to_string(baggageInfo.pid) + " is overweight\n", BLUE, LOG_BAGGAGE_CONTROL);
                        kill(baggageInfo.pid, SIGNAL_PASSENGER_IS_OVERWEIGHT);
                        safeSemop(args.semIDBaggageControlOut, &INC_SEM, 1);
                        // TODO: consider sending PASSENGER_IS_OVERWEIGHT signal to event handler
                        // and then event handler sending signal to passenger
                }
                else
                {
                        vCout("Baggage control: passenger " + std::to_string(baggageInfo.pid) + " is not overweight\n", BLUE, LOG_BAGGAGE_CONTROL);
                        safeSemop(args.semIDBaggageControlOut, &INC_SEM, 1);
                }
        }
}
