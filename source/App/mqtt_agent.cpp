#include "mqtt_agent.h"
#include <websocketpp/base64/base64.hpp>

mqtt_agent::mqtt_agent() {
	g_data::get_mqtt_info(mqtt_address_, mqtt_user_, mqtt_password_, mqtt_client_id_);
	mqtt_client_id_ += "_image_pub";

	client_ = std::make_shared<mqtt_publisher>(mqtt_address_.c_str(), mqtt_user_.c_str(), mqtt_password_.c_str(), mqtt_client_id_.c_str());
	client_->connect();

	sock_ = std::make_shared<zmq::socket_t>(g_data::context(), zmq::socket_type::sub);
	sock_->connect(g_data::stream_for_client_address());
	sock_->set(zmq::sockopt::subscribe, "");

	run_flag_ = true;
	run_th_ = std::thread(&mqtt_agent::run, this);
}

mqtt_agent::~mqtt_agent() {
	run_flag_ = false;
	if (run_th_.joinable()) {
		run_th_.join();
	}
}

void mqtt_agent::run() {
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
				json["command"] = "OP_LPR_STREAM";
				std::string encoded_string = websocketpp::base64_encode(data->data, data->length);
				json["context"] = encoded_string;
			}
			delete []data->data;

			Json::StreamWriterBuilder builder;
			const std::string json_string = Json::writeString(builder, json);
			client_->publish("topic", json_string, 0);
		} else {
			std::this_thread::sleep_for(std::chrono::milliseconds(20));
		}
	}
}