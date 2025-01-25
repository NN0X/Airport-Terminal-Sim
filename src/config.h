#ifndef CONFIG_H
#define CONFIG_H

#define VERBOSE true

#define WORST_CASE_SCENARIO false // INFO: enables the possibility of aggressive passengers ending the program
#define PASSENGER_MAX_SKIPPED 3 // INFO: maximum number of skipped passengers before they become aggressive

#define PLANE_PLACES 10 // INFO: number of places in the plane
#define PLANE_MIN_TIME 10 // in sec INFO: minimum time for plane to come back
#define PLANE_MAX_TIME 100 // in sec INFO: maximum time for plane to come back
#define PLANE_MAX_ALLOWED_BAGGAGE_WEIGHT 75 // kg INFO: maximum allowed baggage weight

#define SECURITY_SELECTOR_DELAY 10 // in ms INFO: delay for security selector to select passenger
#define SECURITY_GATE_MAX_DELAY 50 // in ms INFO: maximum delay for security gate to check passenger
#define SECURITY_GATE_MIN_DELAY 20 // in ms INFO: minimum delay for security gate to check passenger

#define STAIRS_MAX_ALLOWED_OCCUPANCY PLANE_PLACES / 2 // INFO: maximum number of passengers on stairs

#define SEM_INIT_VALUE 0 // INFO: initial value for semaphores
#define SEM_NUMBER 24 // INFO: number of semaphores

#define PASSENGER_GETS_AGGRESSIVE_BAGGAGE_PROBABILITY 0.01 // INFO: probability of passenger getting aggressive due to overweight
#define PASSENGER_GETS_AGGRESSIVE_SECURITY_PROBABILITY 0.05 // INFO: probability of passenger getting aggressive due to dangerous

#endif // CONFIG_H
