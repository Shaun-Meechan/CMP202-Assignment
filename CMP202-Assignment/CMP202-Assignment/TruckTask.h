#pragma once
#include "Task.h"

class TruckTask : public Task
{
public:
    TruckTask(bool delivery);
    void run();
    int getCount();
    truckType getType();
    bool getFinished();
private:
    int count;
    truckType type;
    bool finished = false;
};