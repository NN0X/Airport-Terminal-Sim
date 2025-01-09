#include <iostream>
#include <vector>
#include <cstdint>
#include <unistd.h>

#include "plane.h"
#include "passenger.h"

void genPassengers(std::vector<Passenger> &passengers)
{
	// TODO: generate passengers
}

void genPlanes(std::vector<Plane> &planes)
{
	// TODO: generate planes
}

int secControl()
{
	while (true)
	{
		// do something
	}
}

int dispatcher()
{
	while (true)
	{
		// do something
	}
}

int passenger()
{
}

int plane()
{
}

int main(int argc, char* argv[])
{
	// TODO: checks for argc and argv

	const size_t numPlanes = 10;

	// get this instance pid
	const pid_t mainPID = getpid();

	// fork secControl
	const pid_t secControlPID = fork();

	// fork dispatcher
	const pid_t dispatcherPID = fork();

	std::vector<Passenger> passengersInTerminal;
	std::vector<Plane> planes;

	if (getpid() == mainPID)
	{
		size_t numPassengers = 0;
		numPassengers = std::stoul(argv[1]);
		passengersInTerminal.reserve(numPassengers);
		genPassengers(passengersInTerminal);

		size_t numPlanes = 0;
		numPlanes = std::stoul(argv[2]);
		planes.reserve(numPlanes);
		genPlanes(planes);

		std::cout << "this is main\n";
	}
	else if (getpid() == dispatcherPID)
	{
		std::cout << "this is dispatcher\n";
	}
	else if (getpid() == secControlPID)
	{
		std::cout << "this is secControl\n";
	}

	return 0;
}
