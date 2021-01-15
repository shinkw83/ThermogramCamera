#pragma once

#include "publisher.h"
#include "global.hpp"

class mqtt_agent {
public:
	mqtt_agent();
	~mqtt_agent();

private:
	void run();

private:
	std::string mqtt_address_;
	std::string mqtt_user_;
	std::string mqtt_password_;
	std::string mqtt_client_id_;

	std::shared_ptr<mqtt_publisher> client_;
	std::shared_ptr<zmq::socket_t> sock_;

	std::thread run_th_;
	std::atomic<bool> run_flag_;
};