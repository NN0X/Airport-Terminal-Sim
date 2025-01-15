#include <cstdint>

int eventGenerator()
{
        while (true)
        {
                // INFO: generates random events without input from other processes
                // INFO: sends signals to eventHandler
        }
}

int randomEvent(uint8_t category)
{
        while (true)
        {
                // INFO: generates random events
        }
}

int eventHandler()
{
        while (true)
        {
                // INFO: receives signals from other processes and threads
                // INFO: based on signal, it generates random event of a specific category
                // INFO: sends signals to other processes and threads based on event
        }
}

