#include "websocket_server.h"

websocket_server::websocket_server() {
	run_flag_ = true;
	open_port_ = 8890;

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
				json["command"] = "OP_RASPI_STREAM";
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

}