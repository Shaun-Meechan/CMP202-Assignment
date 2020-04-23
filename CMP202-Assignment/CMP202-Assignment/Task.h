#ifndef TASK_H
#define TASK_H

#include <cstdint>

enum class truckType
{
	outbound,
	inbound
};

/** Abstract base class: a task to be executed. */
class Task
{
public:
	virtual ~Task()
	{
	}

	/** Perform the task. Subclasses must override this. */
	virtual void run() = 0;
	/** Allow us to retrive the count of the Task*/
	virtual int getCount() = 0;
	/** Allow us to retrive the type of the task*/
	virtual truckType getType() = 0;
};

#endif
