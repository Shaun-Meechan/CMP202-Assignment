#pragma once
#include "Task.h"
#include <cstdlib>
#include <iostream>
#include <complex>

class TruckTask : public Task
{
public:
    TruckTask(bool delivery);
    void run();
    int getCount();
    truckType getType();
private:
    int count;
    truckType type;
    bool finished = false;
    void quicksort(float* a, int start, int end);
    int partition(float* a, int start, int end);
};