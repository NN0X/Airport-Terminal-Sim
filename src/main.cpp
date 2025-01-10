#include <iostream>
#include <vector>
#include <cstdint>
#include <unistd.h>

#include "plane.h"
#include "passenger.h"

void initPassengers(size_t num, pid_t mainPID)
{
	pid_t prev = mainPID;
	for (size_t i = 0; i < num; ++i)
	{
		fork();
		pid_t pid = getpid();
		if (pid != prev)
		{
			prev = pid;
		}
		else
		{
			break;
		}

	}

	if (getpid() == mainPID)
	{
		return;
	}

	std::cout << "Passenger process: " << getpid() << std::endl;

	// TODO: increment semaphore value by 1

	// wait for init phase to finish (semaphore)
	// TODO: wait for semaphore value to be set to 0

	// TODO: run passenger tasks here

	if (getpid() != mainPID)
	{
		exit(0);
	}
}

void initPlanes(size_t num, pid_t mainPID)
{
	pid_t prev = mainPID;
	for (size_t i = 0; i < num; ++i)
	{
		fork();
		pid_t pid = getpid();
		if (pid != prev)
		{
			prev = pid;
		}
		else
		{
			break;
		}

	}

	if (getpid() == mainPID)
	{
		return;
	}

	std::cout << "Plane process: " << getpid() << std::endl;

	// TODO: increment semaphore value by 1

	// wait for init phase to finish (semaphore)
	// TODO: wait for semaphore value to be set to 0

	// TODO: run plane tasks here

	if (getpid() != mainPID)
	{
		exit(0);
	}
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

int main(int argc, char* argv[])
{
	// TODO: checks for argc and argv


	std::vector<pid_t> pids(3);
	pids[0] = getpid();

	// main + secControl + dispatcher + n passengers + n planes

	size_t numPassengers = std::stoul(argv[1]);
	size_t numPlanes = std::stoul(argv[2]);


	// TODO: init semaphore with value 0

	initPassengers(numPassengers, pids[0]);
	initPlanes(numPlanes, pids[0]);

	fork();
	pid_t pid = getpid();
	if (pid != pids[0])
	{
		pids[1] = pid;
		fork();
		pid = getpid();
		if (pid != pids[0] && pid != pids[1])
		{
			pids[2] = pid;
		}
	}

	// TODO: check if semaphore is set to numPassengers + numPlanes

	// if semaphore value = numPassengers + numPlanes set semaphore to 0

	// TODO: runtime

	//

	if (getpid() == pids[0])
		std::cout << "Main process: " << getpid() << std::endl;
	else if (getpid() == pids[1])
		std::cout << "SecControl process: " << getpid() << std::endl;
	else if (getpid() == pids[2])
		std::cout << "Dispatcher process: " << getpid() << std::endl;

	return 0;
}
