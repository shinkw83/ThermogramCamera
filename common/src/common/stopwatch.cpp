#include "stopwatch.h"
stopwatch::stopwatch()
{
    m_record_tc = chrono::system_clock::now();
}


stopwatch::~stopwatch()
{
}

double stopwatch::GetElms(stopwatch& record)
{
    chrono::duration<double> el_sec = chrono::system_clock::now() - record.m_record_tc;
    return el_sec.count() * 1000.0;
}


