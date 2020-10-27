#pragma once

#include "global.hpp"
#include "agent_broker.h"

using boost::asio::ip::tcp;

class api_client {
public:
	api_client(boost::asio::io_context &io, agent_broker *broker);
	~api_client();

	tcp::socket &socket();
	void run();
	bool is_run();

private:
	void read();
	void write(xpacket *packet);

	void stream();
	void command_proc();

private:
	tcp::socket sock_;
	std::shared_ptr<zmq::socket_t> stream_sock_;

	std::mutex cam_mutex_;
	std::mutex write_mutex_;
	std::atomic<bool> running_;

	std::thread stream_th_;
	std::atomic<bool> run_flag_;

	std::deque<uint8_t> h_prefix_;
	uint8_t recv_buf_[256];
	uint8_t msg_buf_[256];
	std::size_t buf_pos_;
	xpacket *recv_packet_;

	int recv_step_;

	agent_broker *broker_;
};