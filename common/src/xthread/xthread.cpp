#include "xthread.h"

#include <iostream>

xthread::xthread()
{	
}


xthread::~xthread()
{
	if (m_thread_started == true)
		Stop();
}

void xthread::SetCPS(double cps)
{
	m_cps_lock.lock();
	m_cps = cps;
	m_cp_period = 1.0 / cps;
	m_cps_lock.unlock();
}

int xthread::Start(void* arg)
{
	if (m_thread_started == true)
		return -1;
	m_arg = arg;
	m_stop_flag = false;
	m_last_com_tc = m_last_ok_tc = chrono::system_clock::now();
	m_thread = thread(&xthread::Run, this);
	m_thread_started = true;
	return 0;
}

int xthread::Stop()
{	
    if (m_thread_started == false)
        return -1;
    m_stop_flag = true;
    if (m_detach_at_destroy == true)
        m_thread.detach();
    else
        m_thread.join();
    m_thread_started = false;
    return 0;
}

void xthread::Run()
{
    while (m_stop_flag == false)
    {
        //CPS
        chrono::time_point<chrono::system_clock> now = chrono::system_clock::now();
        {
            m_cps_lock.lock();
            if (m_cps > 0)
            {				
                chrono::duration<double> el_sec = now - m_last_com_tc;

                if (el_sec.count() < m_cp_period)
                {
                    m_cps_lock.unlock();
                    this_thread::sleep_for(chrono::milliseconds(5));
                    continue;
                }
            }
            m_cps_lock.unlock();
        }
        m_state = Proc();
        
        if (m_state >= 0)
        {
            m_last_ok_tc = chrono::system_clock::now();
        }
        else if (m_state == DEATH_CODE)
        {
            m_stop_flag = true;
            m_detach_at_destroy = true;
        }
        m_last_com_tc = now;
    }
}
