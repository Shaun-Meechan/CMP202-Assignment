//We don't have lots of include as QueueFiller has most of these includes anyway, no need to include twice.

#include "Task.h"
#include "TruckTask.h"
#include "QueueFillerTask.h"
#include <list>

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
using std::list;
using namespace concurrency;

//Define the name "the_clock" for the clock we are using
typedef std::chrono::steady_clock the_clock;

//Define the threads
vector<thread*> workerThreads;
std::queue<Task*> tasksQueue;
//Define our queueFillerTask
QueueFillerTask* queueFillerTask;
//Create global variables

//If true program will begin to shutdown
bool finish = false;
//Mutex to protect the queue of tasks
mutex queueMutex;
//Mutex to protect our worker threads vector
mutex workerThreadMutex;
//Bool to put worker threads to sleep
bool noMoreTasks = false;
//Condition variable to add new tasks to the queue
condition_variable queueCV;
//Condition variable to add or remove worker threads
condition_variable workerCV;
//Condition variable to wake or sleep our queue watcher thread
condition_variable queueWatcherCV;
//Bool to find out if we can add tasks to the queue
bool addToQueue = false;
//Used to make sure that the program closes correctly we asked to do so
bool killSwitch = false;
//Function to see if we can change the status of finish
bool changeFinish();
//How many worker threads to make. Used in main
int workerThreadsToMake = 1;

//Function that handles working on tasks. Used by worker threads
void workerThreadFunction()
{
	cout << "Started a worker thread!" << endl;
	Task* currentTask = NULL;
	//Begin working
	while (finish == false)
	{
		unique_lock<mutex> workerLock(workerThreadMutex);
		//Get a task if we don't have one
		//If there are no tasks, put the thread to sleep
		while (currentTask == NULL && noMoreTasks == false)
		{
			queueMutex.lock();
			if (tasksQueue.empty() == false)
			{
				currentTask = tasksQueue.front();
				tasksQueue.pop();
				queueMutex.unlock();
			}
			else
			{
				queueMutex.unlock();
				workerCV.wait(workerLock, []() { return noMoreTasks; });
				break;
			}
		}

		//If no more tasks is true then we stop the thread.
		if (noMoreTasks == true)
		{
			cout << "Worker was told to stop." << endl;
			break;
		}

		cout << "Running task.\n";

		the_clock::time_point start = the_clock::now();

		currentTask->run();

		//Timing code.
		the_clock::time_point end = the_clock::now();
		auto time_taken = duration_cast<microseconds>(end - start).count();
		//timeTaken += time_taken;
		std::cout << "Time taken was " << time_taken << std::endl;
		cout << "Finished task!" << endl;
		currentTask = NULL;
	}
}

//Function that handles adding tasks to our queue. Used by the queue filler thread.
void QueueFillerThreadFunction()
{
	while (finish != true)
	{
		//Lock the queueMutex as we need to add to it.
		unique_lock<mutex> lock(queueMutex);
		//The thread should wait until the manager thread tells it to start
		queueCV.wait(lock, []() { return addToQueue; });
		cout << "Before running task, queue was size: " << tasksQueue.size() << endl;
		tasksQueue = queueFillerTask->run(tasksQueue);
		cout << "After running Queue filler, queue size is: " << tasksQueue.size() << endl;
		cout << "Worker threads left = " << workerThreads.size() << endl;
		//Unlock as we are now done with the queue
		lock.unlock();

		std::cout << "Added to queue successfully. Restarting all worker threads..." << endl;

		////Restart our queueWatcher Thread
		std::unique_lock<mutex> queueWatcherLock(queueMutex);
		queueWatcherLock.unlock();
		queueWatcherCV.notify_one();

		//Restart all 4 worker threads
		for (int i = 0; i < workerThreadsToMake; i++)
		{
			workerThreads.push_back(new thread(workerThreadFunction));
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
		}

		std::cout << "All worker threads restarted!" << endl;
		addToQueue = false;
	}
}

//Function for stopping worker threads and starting the queue filler thread
void threadManager()
{
	while (finish != true)
	{
		//Don't do anything for the first 10 seconds.
		std::this_thread::sleep_for(std::chrono::seconds(10));
		//Lock both mutexes as we will be using them
		std::unique_lock<mutex> lock(queueMutex);
		std::unique_lock<mutex> workerLock(workerThreadMutex);

		std::cout << "We are going to add to the tasks queue. Ending all worker threads!" << endl;

		//Try to change finish to true to stop the worker threads.
		if (changeFinish() == true)
		{
			finish = true;
		}

		cout << "There are " << workerThreads.size() << " worker threads." << endl;
		workerLock.unlock();
		noMoreTasks = true;
		workerCV.notify_all();
		for (unsigned int i = 0; i < workerThreads.size(); i++)
		{
			//Stop all the worker threads so we can add more to the queue using a conditonal variable
			cout << "Stopping worker thread " << i << endl;
			workerThreads[i]->join();
			cout << "Worker thread for " << i << " has stopped!" << endl;
		}

		//We clear the vector in case any threads remain in it.
		workerThreads.clear();

		//Try to change finish back to false to restart the worker threads.
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
		noMoreTasks = false;
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
	bool validData = false;

	do
	{
		cout << "How many worker threads do you want?\n";
		std::cin >> workerThreadsToMake;
		if (std::cin.good())
		{
			validData = true;
		}
		else
		{
			std::cin.clear();
			std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
			cout << "ERROR: Invalid input, please try again" << endl;
		}
	} while (validData == false);
	//Fill our tasks queue with truck tasks (10, 5 of each type)
	for (int i = 0; i < 5; i++)
	{
		tasksQueue.push(new TruckTask(true));
		tasksQueue.push(new TruckTask(false));
	}

	//Start all our worker threads
	for (int i = 0; i < workerThreadsToMake; i++)
	{
		workerThreads.push_back(new thread(workerThreadFunction));
		//std::this_thread::sleep_for(std::chrono::milliseconds(500));
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

	//Start to join all threads and close
	QueueFillerThread.join();
	threadManagerThread.join();

	for (unsigned int i = 0; i < workerThreads.size(); i++)
	{
		workerThreads[i]->join();
	}
	cout << "All worker threads stopped." << endl;

	cout << "Program finished!";
	return 0;
}