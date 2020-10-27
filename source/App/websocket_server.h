#pragma once

#include "global.hpp"

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <websocketpp/base64/base64.hpp>

#include "xpacket.h"
#include "agent_broker.h"

using websocketpp::connection_hdl;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

typedef websocketpp::server<websocketpp::config::asio> web_server;
typedef std::set<connection_hdl, std::owner_less<connection_hdl>> web_connection;

class websocket_server {
public:
	websocket_server(agent_broker *broker);
	~websocket_server();

private:
	void run();
	void recv();

	void on_open(connection_hdl hdl);
	void on_close(connection_hdl hdl);
	void on_message(connection_hdl hdl, web_server::message_ptr msg);

private:
	std::thread run_th_;
	std::thread recv_th_;
	std::atomic<bool> run_flag_;

	std::shared_ptr<zmq::socket_t> sock_;

	web_server server_;
	web_connection connections_;
	std::mutex conn_mutex_;

	int open_port_;
	
	agent_broker *broker_;

};