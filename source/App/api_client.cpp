#include "api_client.h"

api_client::api_client(boost::asio::io_context &io) : sock_(io), proc_sock_(g_data::context(), zmq::socket_type::sub) {
	proc_run_flag_ = true;

	proc_sock_.bind(g_data::api_proc_address());
	proc_sock_.set(zmq::sockopt::subscribe, "");

	recv_step_ = 0;
	buf_pos_ = 0;
	recv_packet_ = nullptr;
	h_prefix_.clear();
}

api_client::~api_client() {
	proc_run_flag_ = false;
	if (proc_th_.joinable()) {
		proc_th_.join();
	}
}

tcp::socket &api_client::socket() {
	return sock_;
}

void api_client::run() {
	running_ = true;

	proc_th_ = std::thread(&api_client::proc, this);
	read();
}

bool api_client::is_run() {
	return running_;
}

void api_client::read() {
	if (recv_step_ == 0) {	// find header prefix
		while (h_prefix_.size() >= xpacket::PREFIX_SIZE) {
			h_prefix_.pop_front();
		}

		boost::asio::async_read(sock_, boost::asio::buffer(recv_buf_, 1),
			[this](const boost::system::error_code &ec, const long unsigned int&)
			{
				if (!ec) {
					h_prefix_.push_back(recv_buf_[0]);
					if (h_prefix_.size() == xpacket::PREFIX_SIZE) {
						if (h_prefix_[0] == 'R' and h_prefix_[1] == 'X' and h_prefix_[2] == 'M' and h_prefix_[3] == 'G') {
							recv_step_ = 1;
							msg_buf_[0] = 'R'; msg_buf_[1] = 'X'; msg_buf_[2] = 'M'; msg_buf_[3] = 'G';
							buf_pos_ = xpacket::PREFIX_SIZE;
							h_prefix_.clear();
							recv_packet_ = new xpacket(256);
						}
					}
					read();
				} else {
					g_data::log(ERROR_LOG_LEVEL, "[api_client] Read prefix error[%s]", ec.message().c_str());
					running_ = false;
				}
			}
		);
	} else if (recv_step_ == 1) {	// recv whole header	
		std::size_t remain_header_size = xpacket::XPACKET_HEADER_SIZE - buf_pos_;
		boost::asio::async_read(sock_, boost::asio::buffer(recv_buf_, remain_header_size),
			[this](const boost::system::error_code &ec, const long unsigned int &bytes_received)
			{
				if (!ec) {
					std::memcpy(msg_buf_ + buf_pos_, recv_buf_, bytes_received);
					buf_pos_ += bytes_received;

					if (buf_pos_ >= xpacket::XPACKET_HEADER_SIZE) {
						std::memcpy(&recv_packet_->m_header, msg_buf_, xpacket::XPACKET_HEADER_SIZE);
						recv_step_ = 2;
					}
					read();
				} else {
					g_data::log(ERROR_LOG_LEVEL, "[api_client] Read remain header error[%s]", ec.message().c_str());
					running_ = false;
				}
			}
		);
	} else if (recv_step_ == 2) {

	} else {
		recv_step_ = 0;
	}
}

void api_client::write(xpacket *packet) {

}

void api_client::proc() {
	while (proc_run_flag_) {
		std::this_thread::sleep_for(std::chrono::milliseconds(30));
	}
}