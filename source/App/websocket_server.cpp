#include "websocket_server.h"
#include "command.h"
#include "raspicam_set.h"

websocket_server::websocket_server(agent_broker *broker) {
	broker_ = broker;
	run_flag_ = true;
	open_port_ = g_data::web_port();

	sock_ = std::make_shared<zmq::socket_t>(g_data::context(), zmq::socket_type::sub);
	sock_->connect(g_data::stream_for_client_address());
	sock_->set(zmq::sockopt::subscribe, "");

	run_th_ = std::thread(&websocket_server::run, this);
	recv_th_ = std::thread(&websocket_server::recv, this);
}

websocket_server::~websocket_server() {
	run_flag_ = false;
	server_.stop();

	if (run_th_.joinable()) {
		run_th_.join();
	}

	if (recv_th_.joinable()) {
		recv_th_.join();
	}
}

void websocket_server::run() {
	while (run_flag_) {
		server_.init_asio();

		server_.clear_access_channels(websocketpp::log::alevel::all);
		server_.set_open_handler(bind(&websocket_server::on_open, this, ::_1));
		server_.set_close_handler(bind(&websocket_server::on_close, this, ::_1));
		server_.set_message_handler(bind(&websocket_server::on_message, this, ::_1, ::_2));
		server_.set_reuse_addr(true);

		boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(), open_port_);
		server_.listen(endpoint);
		server_.start_accept();

		try {
			server_.run();
		} catch (websocketpp::exception &e) {
			g_data::log(ERROR_LOG_LEVEL, "[websocket_server] Accept error.[%s]", e.what());

			server_.stop();
			server_.reset();
		}
	}
}

void websocket_server::recv() {
	while (run_flag_) {
		zmq::message_t message;
		auto res = sock_->recv(message, zmq::recv_flags::dontwait);
		if (res && res.value() > 0) {
			stream_data *data = message.data<stream_data>();

			Json::Value json;
			if (data->type == RASPICAM_IMAGE) {
				json["command"] = "OP_CONTEXT_STREAM";
				std::string encoded_string = websocketpp::base64_encode(data->data, data->length);
				json["context"] = encoded_string;
			} else if (data->type == THERMOGRAM_IMAGE) {

			}
			delete []data->data;

			Json::StreamWriterBuilder builder;
			const std::string json_string = Json::writeString(builder, json);

			std::lock_guard<std::mutex> guard(conn_mutex_);
			for (auto it : connections_) {
				try {
					server_.send(it, json_string, websocketpp::frame::opcode::text);
				} catch (websocketpp::exception &e) {
					g_data::log(ERROR_LOG_LEVEL, "[websocket_server] Send frame error[%s]", e.what());
				}
			}
		} else {
			std::this_thread::sleep_for(std::chrono::milliseconds(20));
		}
	}
}

void websocket_server::on_open(connection_hdl hdl) {
	g_data::log(INFO_LOG_LEVEL, "[websocket_server] Client connected.");
	std::lock_guard<std::mutex> guard(conn_mutex_);

	connections_.insert(hdl);
}

void websocket_server::on_close(connection_hdl hdl) {
	g_data::log(INFO_LOG_LEVEL, "[websocket_server] Client disconnected.");
	std::lock_guard<std::mutex> guard(conn_mutex_);

	connections_.erase(hdl);
}

void websocket_server::on_message(connection_hdl hdl, web_server::message_ptr msg) {
	Json::Value root;
	std::string payload = msg->get_payload();

	Json::CharReaderBuilder builder;
	const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
	const auto string_length = static_cast<int>(payload.length());
	JSONCPP_STRING err;

	if (!reader->parse(payload.c_str(), payload.c_str() + string_length, &root, &err)) {
		g_data::log(ERROR_LOG_LEVEL, "[websocket_server] Message parsing fail.[%s][%s]", payload.c_str(), err.c_str());
		return;
	}

	if (root["command"].asString() == "OP_LPR_CAM_COMMAND") {
		// todo thermogram command
	} else if (root["command"].asString() == "OP_CONTEXT_CAM_COMMAND") {
		xpacket recv_packet;
		recv_packet.m_header.h_command = OP_CONTEXTCAM_COMMAND;
		uint8_t sub_command = (uint8_t)root["sub_command"].asInt();
		recv_packet.PushByte(sub_command);

		switch (sub_command)
		{
		case SET_CONTEXT_GAIN:
		case SET_CONTEXT_EXPOSURE: {
			double value = root["value"].asDouble();
			recv_packet.PushMem((uint8_t *)&value, sizeof(double));
			break;
		}
		case SET_CONTEXT_AUTOGAIN:
		case SET_CONTEXT_AUTOEXPOSURE: {
			uint8_t value = (uint8_t)root["value"].asInt();
			recv_packet.PushByte(value);
			break;
		}
		case GET_CONTEXT_EXPOSURE:
		case GET_CONTEXT_GAIN:
		case GET_CONTEXT_AUTOGAIN:
		case GET_CONTEXT_AUTOEXPOSURE: {
			break;
		}
		
		default:
			g_data::log(ERROR_LOG_LEVEL, "[websocket_server] Unknown sub command[%u]", sub_command);
			return;
		}
		recv_packet.Pack();
		recv_packet.ResetPos();

		xpacket repacket;
		std::weak_ptr<raspicam::RaspiCam_Cv> picam = broker_->picam();
		raspicam_set::proc_command(picam, &recv_packet, &repacket);
		repacket.Pack();

		send_response_message(&repacket);
	}
}

void websocket_server::send_response_message(xpacket *packet) {
	Json::Value json;
	json["command"] = "OP_CONTEXT_CAM_COMMAND";
	uint8_t command;
	packet->PopByte(command);

	switch (command)
	{
		case GET_CONTEXT_GAIN:
		{
			double value;
			packet->PopMem((uint8_t*)&value, sizeof(double));

			json["sub_command"] = "GET_CONTEXT_GAIN";
			json["value"] = value;
			break;
		}
		case GET_CONTEXT_EXPOSURE:
		{
			double value;
			packet->PopMem((uint8_t*)&value, sizeof(double));

			json["sub_command"] = "GET_CONTEXT_EXPOSURE";
			json["value"] = value;
			break;
		}
		case GET_CONTEXT_AUTOGAIN:
		{
			uint8_t value;
			packet->PopByte(value);

			json["sub_command"] = "GET_CONTEXT_AUTOGAIN";
			json["value"] = value;
			break;
		}
		case GET_CONTEXT_AUTOEXPOSURE:
		{
			uint8_t value;
			packet->PopByte(value);

			json["sub_command"] = "GET_CONTEXT_AUTOEXPOSURE";
			json["value"] = value;
			break;
		}
		default:
		{
			break;
		}
	}

	Json::StreamWriterBuilder builder;
	const std::string json_string = Json::writeString(builder, json);

	std::lock_guard<std::mutex> guard(conn_mutex_);
	for (auto it : connections_) {
		try {
			server_.send(it, json_string, websocketpp::frame::opcode::text);
		} catch (websocketpp::exception &e) {
			g_data::log(ERROR_LOG_LEVEL, "[websocket_server] Send response error[%s]", e.what());
		}
	}
}