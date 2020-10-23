#pragma once

#include "global.hpp"

class stream_agent {
public:
	stream_agent();
	~stream_agent();

private:
	void run();

private:
	std::shared_ptr<zmq::socket_t> pull_sock_;
	std::shared_ptr<zmq::socket_t> pub_sock_;

	std::thread run_th_;
	std::atomic<bool> run_flag_;
};