#pragma once

#include "global.hpp"

using boost::asio::ip::tcp;

class api_client {
public:
	api_client(boost::asio::io_context &io);
	~api_client();

	tcp::socket &socket();
	void run();
	bool is_run();

private:
	void read();
	void write(xpacket *packet);

	void proc();

private:
	tcp::socket sock_;
	zmq::socket_t proc_sock_;

	std::mutex write_mutex_;
	std::atomic<bool> running_;
	std::thread proc_th_;
	std::atomic<bool> proc_run_flag_;

	std::deque<uint8_t> h_prefix_;
	char recv_buf_[256];
	char msg_buf_[256];
	std::size_t buf_pos_;
	xpacket *recv_packet_;

	int recv_step_;
};