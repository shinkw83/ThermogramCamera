#pragma once

#include <vector>
#include <map>
#include <set>
#include <mutex>
#include <zmq.hpp>
#include <boost/asio.hpp>
#include <stdarg.h>
#include <json/json.h>
#include <boost/lexical_cast.hpp>
#include "singleton.h"
#include "xthread.h"
#include "xpacket.h"
#include "logger.hpp"

enum {
	RASPICAM_IMAGE = 1,
	THERMOGRAM_IMAGE = 2
};

#define OP_RASPICAM_STREAM		1
#define OP_THERMOGRAM_STREAM	2

typedef struct stream_data {
	int type;
	uint32_t index;
	uint8_t *data;
	unsigned long length;

	stream_data() {
		type = 0;
		index = 0;
		data = nullptr;
		length = 0;
	}
} stream_data;

class g_data : public singleton_T<g_data> {
public:
	g_data() : ctx_() {
		stream_from_camera_address_ = "inproc://stream_from_camera";
		stream_for_client_address_ = "inproc://stream_for_client";

		int api_port_ = 6885;
		int web_port_ = 8890;
	}

	virtual ~g_data() {
		if (logmgr_ != nullptr) {
			delete logmgr_;
		}
	}

	static void init(std::string log_path) {
		g_data *data = g_data::GetInstance();
		data->logmgr_ = new LogMgrC(log_path.c_str(), "cam_agent");
	}

	static zmq::context_t &context() {
		g_data *data = g_data::GetInstance();
		return data->ctx_;
	}

	static const char *stream_from_camera_address() {
		g_data *data = g_data::GetInstance();
		return data->stream_from_camera_address_.c_str();
	}

	static const char *stream_for_client_address() {
		g_data *data = g_data::GetInstance();
		return data->stream_for_client_address_.c_str();
	}

	static void set_web_port(int port) {
		g_data *data = g_data::GetInstance();
		data->web_port_ = port;
	}

	static int web_port() {
		g_data *data = g_data::GetInstance();
		return data->web_port_;
	}

	static void set_api_port(int port) {
		g_data *data = g_data::GetInstance();
		data->api_port_ = port;
	}

	static int api_port() {
		g_data *data = g_data::GetInstance();
		return data->api_port_;
	}

	static void set_log_level(int level) {
		g_data *data = g_data::GetInstance();
		if (data->logmgr_ != nullptr) {
			data->logmgr_->setLogLevel(level);
		}
	}

	static void log(int level, const char *format, ...) {
		g_data *data = g_data::GetInstance();
		if (format == nullptr) return;
		if (data->logmgr_ == nullptr) return;

		char logstr[256] = {0, };
		va_list ap;
		va_start(ap, format);
		vsnprintf(logstr, sizeof(logstr), format, ap);
		va_end(ap);

		switch (level) {
		case TRACE_LOG_LEVEL:
			LOG4CPLUS_TRACE(data->logmgr_->get(), logstr);
				break;
		case DEBUG_LOG_LEVEL:
			LOG4CPLUS_DEBUG(data->logmgr_->get(), logstr);
			break;
		case INFO_LOG_LEVEL:
			LOG4CPLUS_INFO(data->logmgr_->get(), logstr);
			break;
		case WARN_LOG_LEVEL:
			LOG4CPLUS_WARN(data->logmgr_->get(), logstr);
			break;
		default:
			LOG4CPLUS_ERROR(data->logmgr_->get(), logstr);
			break;
		}
	}

private:
	zmq::context_t ctx_;

	LogMgrC *logmgr_ = nullptr;

	int api_port_;
	int web_port_;

	std::string stream_from_camera_address_;
	std::string stream_for_client_address_;
};