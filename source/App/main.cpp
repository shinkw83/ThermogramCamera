#include "global.hpp"
#include "config.h"
#include "raspicam_agent.h"
#include "accept_manager.h"
#include "websocket_server.h"

void print_usage() {
	std::cout << "Usage: cam_agent [-c configuration_file_path]" << std::endl;
	std::cout << "   ex) cam_agent -c home/pi/cam_agent/config/cam_agent.ini" << std::endl;
	exit(0);
}

void init_config(std::string path) {
	Config cfg;
	if (cfg.init(path.c_str()) == false) {
		std::cout << "Configuration file path is wrong. Check file path." << std::endl;
		exit(-1);
	}

	std::string log_path = cfg.str("CONFIG", "LOG_PATH", "");
	if (log_path.empty()) {
		std::cout << "Log file path is empty. Check log file path." << std::endl;
		exit(-1);
	}
	g_data::init(log_path);

	std::string log_level = cfg.str("CONFIG", "LOG_LEVEL", "DEBUG");
	if (log_level == "DEBUG") {
		g_data::set_log_level(DEBUG_LOG_LEVEL);
	} else if (log_level == "INFO") {
		g_data::set_log_level(INFO_LOG_LEVEL);
	} else if (log_level == "WARN") {
		g_data::set_log_level(WARN_LOG_LEVEL);
	} else if (log_level == "ERROR") {
		g_data::set_log_level(ERROR_LOG_LEVEL);
	}

	int api_port = cfg.getiValue("OPEN_PORT", "API", 6885);
	g_data::set_api_port(api_port);
	int web_port = cfg.getiValue("OPEN_PORT", "WEB", 8890);
	g_data::set_web_port(web_port);

	g_data::log(INFO_LOG_LEVEL, "[Config] Websocket Port : %d", web_port);
	g_data::log(INFO_LOG_LEVEL, "[Config] API Port : %d", api_port);
}

int main(int argc, char **argv) {
	int n = 0;
	std::string config_path = "../config/cam_agent.ini";
	while ((n = getopt(argc, argv, "hc:")) != EOF) {
		switch (n)
		{
		case 'h':
			print_usage();
			break;
		
		case 'c':
			config_path = optarg;
			break;

		default:
			print_usage();
			break;
		}
	}

	init_config(config_path);

	agent_broker broker_;
	raspicam_agent picam_th_(&broker_);
	accept_manager accept_th_(g_data::api_port(), &broker_);
	websocket_server websocket_th_(&broker_);

	while (true) {
		std::this_thread::sleep_for(std::chrono::seconds(3));
	}

	return 0;
}