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
#include "global.hpp"

class publisher_callback : public virtual mqtt::callback, public virtual mqtt::iaction_listener {
public:
	publisher_callback(mqtt::async_client &cli, mqtt::connect_options &opt);

private:
	void reconnect();
	void on_failure(const mqtt::token &tok) override;
	void on_success(const mqtt::token& tok) override {}
	void connected(const std::string& cause) override;
	void connection_lost(const std::string& cause) override;
	void delivery_complete(mqtt::delivery_token_ptr token) override {}

private:
	mqtt::async_client &client_;
	mqtt::connect_options &opts_;
};

class mqtt_publisher {
public:
	mqtt_publisher(const char *address, const char *user, const char *password, const char *client_id);

	void connect();
	void disconnect();

	void publish(const std::string &topic, const std::string &payload, const int &qos);

private:
	mqtt::async_client client_;
	mqtt::connect_options opts_;
	publisher_callback cb_;

	std::string address_;
};