#include "subscriber.h"

void subscribe_listener::on_failure(const mqtt::token &tok) {
	g_data::log(ERROR_LOG_LEVEL, "[Subscriber] %s failure for token[%d]", name_.c_str(), tok.get_message_id());
}

void subscribe_listener::on_success(const mqtt::token &tok) {
	g_data::log(INFO_LOG_LEVEL, "[Subscriber] %s success for token[%d]", name_.c_str(), tok.get_message_id());
	auto top = tok.get_topics();
	if (top && !top->empty()) {
		g_data::log(INFO_LOG_LEVEL, "[Subscriber] %s token[%d] topic[%s]", name_.c_str(), tok.get_message_id(), (*top)[0].c_str());
	}
}

subscriber_callback::subscriber_callback(mqtt::async_client &cli, mqtt::connect_options &opt, zmq::context_t &ctx)
 : client_(cli), opts_(opt), listener_("Subscription"), ctx_(ctx), sock_(ctx_, zmq::socket_type::push) {
	 sock_.bind(ZMQ_MQTT_SUBSCRIBE_ADDRESS);
}

void subscriber_callback::set_topic(const std::vector<std::string> &topics, const std::vector<int> &qos) {
	topics_ = topics;
	qos_ = qos;

	if (topics.size() != qos.size()) {
		g_data::log(WARN_LOG_LEVEL, "[subscriber_callback] Topic count != QoS count");
		g_data::log(WARN_LOG_LEVEL, "[subscriber_callback] All topic qos set 0.");
		qos_.clear();
		for (auto it : topics_) {
			qos_.push_back(0);
		}
	}
}

void subscriber_callback::reconnect() {
	std::this_thread::sleep_for(std::chrono::milliseconds(2500));
	try {
		g_data::log(INFO_LOG_LEVEL, "[subscriber_callback] Reconnecting to the MQTT server...");
		client_.connect(opts_, nullptr, *this);
	} catch (const mqtt::exception &exc) {
		g_data::log(ERROR_LOG_LEVEL, "[subscriber_callback] Unable to reconnect to MQTT server[what:%s]", exc.what());
	}
}

void subscriber_callback::on_failure(const mqtt::token &tok) {
	g_data::log(ERROR_LOG_LEVEL, "[subscriber_callback] Connection attempt failed.");
	reconnect();
}

void subscriber_callback::connected(const std::string& cause) {
	g_data::log(INFO_LOG_LEVEL, "[subscriber_callback] Connection success. Try subscribe.");
	for (size_t i = 0; i < topics_.size(); i++) {
		client_.subscribe(topics_[i], qos_[i], nullptr, listener_);
	}
}

void subscriber_callback::connection_lost(const std::string& cause) {
	g_data::log(ERROR_LOG_LEVEL, "[subscriber_callback] Connection lost.[cause:%s]", cause.c_str());
	reconnect();
}

void subscriber_callback::message_arrived(mqtt::const_message_ptr msg) {
	Json::Value root;
	root["topic"] = msg->get_topic();
	root["payload"] = msg->to_string();

	std::string json = Json::writeString(writer_, root);
	zmq::message_t jsonmsg(json.data(), json.size());

	sock_.send(jsonmsg, zmq::send_flags::dontwait);
}

mqtt_subscriber::mqtt_subscriber(const char *address, const char *user, const char *password, const char *client_id)
 : client_(address, client_id), opts_(), ctx_(), cb_(client_, opts_, ctx_), address_(address) {
	 opts_.set_keep_alive_interval(20);
	 opts_.set_clean_session(true);
	 opts_.set_user_name(user);
	 opts_.set_password(password);

	 client_.set_callback(cb_);
}

void mqtt_subscriber::connect() {
	try {
		g_data::log(INFO_LOG_LEVEL, "[mqtt_subscriber] Connecting to the MQTT server...[address:%s]", address_.c_str());
		client_.connect(opts_, nullptr, cb_);
	} catch (const mqtt::exception &exc) {
		g_data::log(ERROR_LOG_LEVEL, "[mqtt_subscriber] Unable to connect to MQTT server[address:%s][what:%s]", address_.c_str(), exc.what());
	}
}

void mqtt_subscriber::disconnect() {
	g_data::log(INFO_LOG_LEVEL, "[mqtt_subscriber] Disconnecting from the MQTT server...[address:%s]", address_.c_str());
	client_.disconnect()->wait();
}

void mqtt_subscriber::set_topic(const std::vector<std::string> &topics, const std::vector<int> &qos) {
	cb_.set_topic(topics, qos);
}