#include "TruckTask.h"


TruckTask::TruckTask(bool delivery)
{
	if (delivery == true)
	{
		count = 50;
		type = truckType::inbound;
	}
	else
	{
		count = 0;
		type = truckType::outbound;
	}
}

void TruckTask::run()
{
	//We are running this function in a loop so we can just do it this way (maybe use atomic?)
	if (type == truckType::inbound)
	{
		count--;
	}
	else
	{
		count++;
	}
}

int TruckTask::getCount()
{
	return count;
}

truckType TruckTask::getType()
{
	return type;
}

