#ifndef EVENTS_H
#define EVENTS_H

#include <cstdint>

enum class EventCategory : uint8_t
{
        // TODO: create event categories
};

int eventGenerator();
int randomEvent(uint8_t category);
int eventHandler();

#endif // EVENTS_H
