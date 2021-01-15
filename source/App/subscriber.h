#pragma once

/*******************************************************************************
 * Copyright (c) 2013-2017 Frank Pagliughi <fpagliughi@mindspring.com>
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v10.html
 * and the Eclipse Distribution License is available at
 *   http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Frank Pagliughi - initial implementation and documentation
 *******************************************************************************/

#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <cctype>
#include <thread>
#include <chrono>
#include "mqtt/async_client.h"
#include "zmq.hpp"
#include "global.hpp"

#define ZMQ_MQTT_SUBSCRIBE_ADDRESS		"inproc://mqtt_subscribe"

class subscribe_listener : public virtual mqtt::iaction_listener {
public:
	subscribe_listener(const std::string &name) : name_(name) {}

private:
	void on_failure(const mqtt::token &tok) override;
	void on_success(const mqtt::token &tok) override;

private:
	std::string name_;
};

class subscriber_callback : public virtual mqtt::callback, public virtual mqtt::iaction_listener {
public:
	subscriber_callback(mqtt::async_client &cli, mqtt::connect_options &opt, zmq::context_t &ctx);

	void set_topic(const std::vector<std::string> &topics, const std::vector<int> &qos);

private:
	void reconnect();
	void on_failure(const mqtt::token &tok) override;
	void on_success(const mqtt::token& tok) override {}
	void connected(const std::string& cause) override;
	void connection_lost(const std::string& cause) override;
	void message_arrived(mqtt::const_message_ptr msg) override;
	void delivery_complete(mqtt::delivery_token_ptr token) override {}

private:
	mqtt::async_client &client_;
	mqtt::connect_options &opts_;
	subscribe_listener listener_;

	zmq::context_t &ctx_;
	zmq::socket_t sock_;

	std::vector<std::string> topics_;
	std::vector<int> qos_;

	Json::StreamWriterBuilder writer_;
};

class mqtt_subscriber {
public:
	mqtt_subscriber(const char *address, const char *user, const char *password, const char *client_id);

	void set_topic(const std::vector<std::string> &topics, const std::vector<int> &qos);
	void connect();
	void disconnect();

	zmq::context_t &context() { return ctx_; }

private:
	mqtt::async_client client_;
	mqtt::connect_options opts_;
	zmq::context_t ctx_;
	subscriber_callback cb_;

	std::string address_;
};