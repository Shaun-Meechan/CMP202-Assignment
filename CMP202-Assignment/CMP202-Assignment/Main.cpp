//We don't have lots of include as QueueFiller has most of these includes anyway, no need to include twice.

#include "Task.h"
#include "TruckTask.h"
#include "QueueFillerTask.h"

//Import what we need from the standard library
using std::chrono::duration_cast;
using std::chrono::milliseconds;
using std::cout;
using std::endl;
using std::mutex;
using std::vector;
using std::thread;
using std::condition_variable;
using std::unique_lock;
using namespace concurrency;

//Define the name "the_clock" for the clock we are using
typedef std::chrono::steady_clock the_clock;

//Define the threads
vector<thread*> workerThreads;
std::queue<Task*> tasksQueue;
QueueFillerTask* queueFillerTask;

//Create global variables
bool finish = false;
mutex queueMutex;
condition_variable queueCV;
bool addToQueue = false;
bool killSwitch = false;
bool changeFinish();

void workerThreadFunction()
{
	cout << "Started a worker thread!" << endl;
	Task* currentTask = NULL;
	while (finish == false)
	{
		//Get a task if we don't have one
		while (currentTask == NULL)
		{
			queueMutex.lock();
			if (tasksQueue.empty() == false)
			{
				currentTask = tasksQueue.front();
				tasksQueue.pop();
			}
			else
			{
				currentTask = NULL;
			}
			queueMutex.unlock();
		}
		//Get the type of truck
		if (currentTask->getType() == truckType::inbound)
		{
			cout << "We are dealing with a inbound truck!" << endl;
			while (currentTask->getCount() > 0)
			{
				currentTask->run();
				std::this_thread::sleep_for(std::chrono::seconds(1));
			}
		}
		else
		{
			cout << "We are dealing with a outbound truck!" << endl;
			while (currentTask->getCount() < 50)
			{
				currentTask->run();
				std::this_thread::sleep_for(std::chrono::seconds(1));
			}
		}
		cout << "Finished task!" << endl;
		currentTask = NULL;
	}
}

void QueueFillerThreadFunction()
{
	while (finish != true)
	{
		unique_lock<mutex> lock(queueMutex);
		queueCV.wait(lock, []() { return addToQueue; });
		cout << "Before running task, queue was size: " << tasksQueue.size() << endl;
		//Lock as we need to add to the queue.
		tasksQueue = queueFillerTask->run(tasksQueue);
		cout << "After running Queue filler, queue size is: " << tasksQueue.size() << endl;
		cout << "Worker threads left = " << workerThreads.size() << endl;
		//Unlock as we are now done with the queue
		lock.unlock();

		std::cout << "Added to queue successfully. Restarting all worker threads..." << endl;

		//Restart all 4 worker threads
		for (int i = 0; i < 4; i++)
		{
			workerThreads.push_back(new thread(workerThreadFunction));
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
		}

		std::cout << "All worker threads restarted!" << endl;
		addToQueue = false;
	}
}

void threadManager()
{
	while (finish != true)
	{
		//Don't do anything for the first 20 seconds.
		std::this_thread::sleep_for(std::chrono::seconds(20));
		std::unique_lock<mutex> lock(queueMutex);

		std::cout << "We are going to add to the tasks queue. Ending all worker threads!" << endl;

		//Find out if the program is trying to close.
		if (changeFinish() == true)
		{
			finish = true;
		}

		cout << "There are " << workerThreads.size() << " worker threads." << endl;
		for (int i = 0; i < workerThreads.size(); i++)
		{
			//Stop all the worker threads so we can add more to the queue using a conditonal variable
			cout << "Stopping worker thread " << i << endl;
			workerThreads[i]->join();
			cout << "Worker thread for " << i << " has stopped!" << endl;
		}

		//We clear the vector in case any threads remain in it.
		workerThreads.clear();

		//Find out if the program is trying to close.
		if (changeFinish() == true)
		{
			finish = false;
		}

		cout << "All worker threads should be stopped." << endl;
		//CV pass into queue filler.
		cout << "Add to queue has been set to true." << endl;
		addToQueue = true;
		//We use notify one because only one thread should be waiting.
		cout << "We are notifying all waiting threads." << endl;
		lock.unlock();
		queueCV.notify_one();
	}
}

//Check to see if we can change the status of finish based on killSwitch value
//If kill switch equals true finish state cannot be changed and all threads must be stopped.
//If kill switch equals false then the value of false can be changed.
bool changeFinish()
{
	if (killSwitch == true)
	{
		return false;
	}
	else
	{
		return true;
	}
}

int main()
{
	std::string value = "";
	//Fill our tasks queue with truck tasks (this could be done parallel)
	for (int i = 0; i < 4; i++)
	{
		tasksQueue.push(new TruckTask(true));
		tasksQueue.push(new TruckTask(false));
	}

	//Start all our worker threads (this could be done parallel)
	for (int i = 0; i < 4; i++)
	{
		workerThreads.push_back(new thread(workerThreadFunction));
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
	}

	thread QueueFillerThread(QueueFillerThreadFunction);

	thread threadManagerThread(threadManager);

	cout << "Enter 's' at any time to end the program" << endl;
	while (finish == false)
	{
		std::cin >> value;

		if (value == "s")
		{
			killSwitch = true;
			finish = true;
			cout << "Finishing thread jobs. This should take 50 seconds at most." << endl;
		}
		//Debug commands
		if (value == "getQueueLength")
		{
			cout << "Tasks queue is currently: " << tasksQueue.size() << endl;
		}
		if (value == "getWorkerThreads")
		{
			cout << "Worker threads count: " << workerThreads.size() << endl;
		}
		if (value == "getAddToQueue")
		{
			cout << "Add to queue is: " << addToQueue << endl;
		}
	}

	QueueFillerThread.join();
	threadManagerThread.join();

	for (int i = 0; i < workerThreads.size(); i++)
	{
		workerThreads[i]->join();
	}
	cout << "All worker threads stopped." << endl;

	cout << "Program finished!";
	return 0;
}