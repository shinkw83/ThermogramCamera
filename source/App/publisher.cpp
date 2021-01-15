#include "publisher.h"

publisher_callback::publisher_callback(mqtt::async_client &cli, mqtt::connect_options &opt)
 : client_(cli), opts_(opt) {
}

void publisher_callback::reconnect() {
	std::this_thread::sleep_for(std::chrono::milliseconds(2500));
	try {
		g_data::log(INFO_LOG_LEVEL, "[publisher_callback] Reconnecting to the MQTT server...");
		client_.connect(opts_, nullptr, *this);
	} catch (const mqtt::exception &exc) {
		g_data::log(ERROR_LOG_LEVEL, "[publisher_callback] Unable to reconnect to MQTT server[what:%s]", exc.what());
	}
}

void publisher_callback::on_failure(const mqtt::token &tok) {
	g_data::log(ERROR_LOG_LEVEL, "[publisher_callback] Connection attempt failed.");
	reconnect();
}

void publisher_callback::connected(const std::string& cause) {
	g_data::log(INFO_LOG_LEVEL, "[publisher_callback] Connection success.");
}

void publisher_callback::connection_lost(const std::string& cause) {
	g_data::log(ERROR_LOG_LEVEL, "[publisher_callback] Connection lost.[cause:%s]", cause.c_str());
	reconnect();
}

mqtt_publisher::mqtt_publisher(const char *address, const char *user, const char *password, const char *client_id)
 : client_(address, client_id), opts_(), cb_(client_, opts_), address_(address) {
	 opts_.set_keep_alive_interval(20);
	 opts_.set_clean_session(true);
	 opts_.set_user_name(user);
	 opts_.set_password(password);

	 client_.set_callback(cb_);
}

void mqtt_publisher::connect() {
	try {
		g_data::log(INFO_LOG_LEVEL, "[mqtt_publisher] Connecting to the MQTT server...[address:%s]", address_.c_str());
		client_.connect(opts_, nullptr, cb_);
	} catch (const mqtt::exception &exc) {
		g_data::log(ERROR_LOG_LEVEL, "[mqtt_publisher] Unable to connect to MQTT server[address:%s][what:%s]", address_.c_str(), exc.what());
	}
}

void mqtt_publisher::disconnect() {
	g_data::log(INFO_LOG_LEVEL, "[mqtt_publisher] Disconnecting from the MQTT server...[address:%s]", address_.c_str());
	client_.disconnect()->wait();
}

void mqtt_publisher::publish(const std::string &topic, const std::string &payload, const int &qos) {
	if (client_.is_connected()) {
		mqtt::message_ptr pubmsg = mqtt::make_message(topic, payload.data());
		pubmsg->set_qos(qos);
		client_.publish(pubmsg);
	}
}