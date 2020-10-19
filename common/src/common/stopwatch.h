#pragma once

#include <chrono>

using namespace std;

class stopwatch
{
public:
    stopwatch();
    ~stopwatch();

    static double GetElms(stopwatch& record);

private:
    chrono::time_point<chrono::system_clock> m_record_tc;
};

