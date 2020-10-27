#include "api_client.h"
#include "command.h"
#include "raspicam_set.h"

api_client::api_client(boost::asio::io_context &io, agent_broker *broker) : sock_(io) {
	broker_ = broker;
	run_flag_ = true;

	stream_sock_ = std::make_shared<zmq::socket_t>(g_data::context(), zmq::socket_type::sub);
	stream_sock_->connect(g_data::stream_for_client_address());
	stream_sock_->set(zmq::sockopt::subscribe, "");

	recv_step_ = 0;
	buf_pos_ = 0;
	recv_packet_ = nullptr;
	h_prefix_.clear();
}

api_client::~api_client() {
	run_flag_ = false;
	if (stream_th_.joinable()) {
		stream_th_.join();
	}

	if (recv_packet_ != nullptr) {
		delete recv_packet_;
		recv_packet_ = nullptr;
	}
}

tcp::socket &api_client::socket() {
	return sock_;
}

void api_client::run() {
	running_ = true;

	stream_th_ = std::thread(&api_client::stream, this);
	read();
}

bool api_client::is_run() {
	return running_;
}

void api_client::read() {
	if (running_ == false) {
		return;
	}

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
		std::size_t remain_packet_size = recv_packet_->m_header.h_packet_size - buf_pos_;
		if (remain_packet_size > 256) {
			running_ = false;
			return;
		}
		boost::asio::async_read(sock_, boost::asio::buffer(recv_buf_, remain_packet_size),
			[this](const boost::system::error_code &ec, const long unsigned int &bytes_received)
			{
				if (ec) {
					g_data::log(ERROR_LOG_LEVEL, "[api_client] Read body error[%s]", ec.message().c_str());
					running_ = false;
					return;
				}

				std::memcpy(msg_buf_ + buf_pos_, recv_buf_, bytes_received);
				buf_pos_ += bytes_received;

				if (buf_pos_ >= recv_packet_->m_header.h_packet_size) {
					if (msg_buf_[buf_pos_ - 2] == 0xFE and msg_buf_[buf_pos_ = 1] == 0xFF) {
						if (recv_packet_->m_header.h_packet_size > xpacket::XPACKET_HEADER_SIZE + xpacket::XPACKET_TAIL_SIZE) {
							recv_packet_->PushMem(msg_buf_ + xpacket::XPACKET_HEADER_SIZE, 
								recv_packet_->m_header.h_packet_size - xpacket::XPACKET_HEADER_SIZE - xpacket::XPACKET_TAIL_SIZE);
						}
						recv_packet_->Pack();
						command_proc();
					}
					buf_pos_ = 0;
					recv_step_ = 0;
				}
				read();
			}
		);
	} else {
		recv_step_ = 0;
	}
}

void api_client::command_proc() {
	recv_packet_->ResetPos();
	uint8_t command = recv_packet_->m_header.h_command;

	if (command == OP_CONTEXTCAM_COMMAND) {
		xpacket repacket;
		std::weak_ptr<raspicam::RaspiCam_Cv> picam = broker_->picam();
		raspicam_set::proc_command(picam, recv_packet_, &repacket);
		repacket.Pack();

		std::lock_guard<std::mutex> guard(cam_mutex_);
		write(&repacket);
	}
	delete recv_packet_;
	recv_packet_ = nullptr;
}

void api_client::write(xpacket *packet) {
	if (running_ == false) {
		return;
	}

	write_mutex_.lock();
	boost::asio::async_write(sock_, boost::asio::buffer(packet->GetBuf(), packet->GetTotalPackSize()), 
		[this](const boost::system::error_code &ec, const long unsigned int&)
		{
			if (ec) {
				g_data::log(ERROR_LOG_LEVEL, "[api_client] Send packet error[%s]", ec.message().c_str());
				running_ = false;
			}
			write_mutex_.unlock();
		}
	);
}

void api_client::stream() {
	while (run_flag_) {
		zmq::message_t message;
		auto res = stream_sock_->recv(message, zmq::recv_flags::dontwait);
		if (res && res.value() > 0) {
			stream_data *data = message.data<stream_data>();
			xpacket stream_packet(data->length + 1024);
			if (data->type == RASPICAM_IMAGE) {
				stream_packet.m_header.h_command = OP_CONTEXT_STREAM;
			} else if (data->type == THERMOGRAM_IMAGE) {
				stream_packet.m_header.h_command = OP_LPR_STREAM;
			}
			stream_packet.m_header.h_index = data->index;
			stream_packet.PushDWord(data->length);
			stream_packet.PushMem(data->data, data->length);
			stream_packet.Pack();

			std::vector<xpacket *> slices;
			xpacket::Split(&stream_packet, slices, 1024);

			std::lock_guard<std::mutex> guard(cam_mutex_);
			for (size_t i = 0; i < slices.size(); i++) {
				write(slices[i]);
				delete slices[i];
			}
		} else {
			std::this_thread::sleep_for(std::chrono::milliseconds(30));
		}
	}
}
