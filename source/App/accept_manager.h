#pragma once

#include "global.hpp"
#include "api_client.h"

using boost::asio::ip::tcp;

class accept_manager {
public:
	accept_manager(int port);
	~accept_manager();

private:
	void accept();
	void run();
	void check_client();

private:
	boost::asio::io_context io_;
	tcp::acceptor acceptor_;

	std::thread run_th_;
	std::thread check_client_th_;
	std::mutex client_mutex_;
	std::atomic<bool> run_flag_;

	std::vector<api_client *> clients_;
};