#include "QueueFillerTask.h"

std::queue<Task*> QueueFillerTask::run(std::queue<Task*> tasksQueue)
{
	//Int to determine how many trucks to add.
	int randomNumber = rand() % 10;
	//Int to determine if the truck should be "inbound" or "outbound"
	int r2 = rand() % 2;
	//Make this parralel
	if (r2 == 0)
	{
		std::cout << "We are adding a inbound truck!" << std::endl;
		for (int i = 0; i < randomNumber; i++)
		{
			tasksQueue.push(new TruckTask(true));
		}
	}
	else
	{
		std::cout << "We are adding a outbound truck!" << std::endl;
		for (int i = 0; i < randomNumber; i++)
		{
			tasksQueue.push(new TruckTask(false));
		}
	}

	//Reset our ints.
	randomNumber = NULL;
	r2 = NULL;

	return tasksQueue;
}
