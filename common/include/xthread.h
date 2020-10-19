#pragma once

#include <thread>
#include <chrono>
#include <mutex>
#include <functional>
#include <iostream>

using namespace std;
const int ZETAVU_PARKING_TYPE = 1;
const int ZETAVU_TYPE = 0;

class xthread
{
public:
	xthread();
	virtual ~xthread();

	void SetCPS(double cps);

	int Start(void* arg);
	int Stop();

	int GetState() { return m_state; }
	void SetState(int status) {m_alive_state = status;}
    bool IsStarted() { return m_thread_started; }
	bool IsStopped() { return m_stop_flag; }
	void SetStopped(bool status) {m_stop_flag = status;}
    const static int DEATH_CODE = -4444;

	chrono::time_point<chrono::system_clock> m_last_com_tc, m_last_ok_tc;

protected:
	void *m_arg;
	int m_state = 0;
	int m_alive_state = 0;
    bool m_detach_at_destroy = false;

private:	
	double m_cps = -1;
	double m_cp_period;
	mutex m_cps_lock;

	bool m_stop_flag = false;

	bool m_thread_started = false;
	std::thread m_thread;

	void Run();
	
	virtual int Proc() { return 0; }
};

