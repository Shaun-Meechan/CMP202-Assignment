#include "TruckTask.h"
using namespace std;

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
	float numbers[1000];

	for (unsigned int i = 0; i < 1000; i++)
	{
		//numbers[i] = rand() % 999;
		numbers[i] = static_cast <float> (rand()) / (static_cast<float>(RAND_MAX / 1000));
	}

	quicksort(numbers, numbers[0], numbers[999] - 1);
}

int TruckTask::getCount()
{
	return count;
}

truckType TruckTask::getType()
{
	return type;
}

int TruckTask::partition(float* a, int start, int end)
{
	long pivot = a[end];

	long PartionIndex = start;
	long i, t;

	//Check if the array value is less than the pivot.
	//We then place is at the left side by swapping
	for (i = start; i < end; i++)
	{
		if (a[i] <= pivot)
		{
			t = a[i];
			a[i] = a[PartionIndex];
			a[PartionIndex] = t;
			PartionIndex++;
		}
	}

	//Exchnage the value of pivot and the value at the index
	t = a[end];
	a[end] = a[PartionIndex];
	a[PartionIndex] = t;

	//Return the last pivot index
	return PartionIndex;
}

void TruckTask::quicksort(float* a, int start, int end)
{
	if (start < end)
	{
		int PartitionIndex = partition(a, start, end);
		quicksort(a, start, PartitionIndex - 1);
		quicksort(a, PartitionIndex + 1, end);
	}
}
