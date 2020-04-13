#pragma once

#include "Task.h"
#include "TruckTask.h"
#include <queue>
#include <ppl.h>
#include <chrono>
#include <iostream>

using namespace concurrency;
using std::chrono::microseconds;
using std::chrono::duration_cast;

class QueueFillerTask : public Task
{
public:
	std::queue<Task*> run(std::queue<Task*> tasksQueue);
private:
	typedef std::chrono::steady_clock the_clock;
};

