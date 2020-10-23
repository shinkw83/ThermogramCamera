#include "stream_agent.h"

stream_agent::stream_agent() {
	run_flag_ = true;

	pull_sock_ = std::make_shared<zmq::socket_t>(g_data::context(), zmq::socket_type::pull);
	pull_sock_->bind(g_data::stream_from_camera_address());

	pub_sock_ = std::make_shared<zmq::socket_t>(g_data::context(), zmq::socket_type::pub);
	pub_sock_->bind(g_data::stream_for_client_address());

	run_th_ = std::thread(&stream_agent::run, this);
}

stream_agent::~stream_agent() {
	run_flag_ = false;

	if (run_th_.joinable()) {
		run_th_.join();
	}
}

void stream_agent::run() {
	while (run_flag_) {
		zmq::message_t message;
		auto res = pull_sock_->recv(message, zmq::recv_flags::dontwait);
		if (res && res.value() > 0) {
			pub_sock_->send(message, zmq::send_flags::dontwait);
		} else {
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
	}
}