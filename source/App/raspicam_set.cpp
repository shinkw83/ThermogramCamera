#include "raspicam_set.h"
#include "command.h"

void raspicam_set::proc_command(std::weak_ptr<raspicam::RaspiCam_Cv> picam, xpacket *packet, xpacket *repacket) {
	uint8_t sub_command = 0;
	if (packet->PopByte(sub_command) < 0) {
		return;
	}

	switch (sub_command)
	{
	case SET_CONTEXT_GAIN:
	case GET_CONTEXT_GAIN:
	case SET_CONTEXT_EXPOSURE:
	case GET_CONTEXT_EXPOSURE: {
		double value;
		if (packet->PopMem((uint8_t *)&value, sizeof(double)) < 0) {
			return;
		}
		set_picam_param(picam, sub_command, value, repacket);
		break;
	}
	case SET_CONTEXT_AUTOGAIN:
	case GET_CONTEXT_AUTOGAIN:
	case SET_CONTEXT_AUTOEXPOSURE:
	case GET_CONTEXT_AUTOEXPOSURE: {
		uint8_t status_value;
		if (packet->PopByte(status_value) < 0) {
			return;
		}
		set_picam_param(picam, sub_command, status_value, repacket);
		break;
	}
	default:
		g_data::log(ERROR_LOG_LEVEL, "[raspicam_set] Unknown command [%u]", sub_command);
		break;
	}
}

void raspicam_set::set_picam_param(std::weak_ptr<raspicam::RaspiCam_Cv> picam, uint8_t command, double value, xpacket *repacket) {
	switch (command) {
		case SET_CONTEXT_GAIN:
			set_gain_param(picam, value, repacket);
			break;
		case GET_CONTEXT_GAIN:
			get_gain_param(picam, repacket);
			break;
		case SET_CONTEXT_EXPOSURE:
			set_exposure_time(picam, value, repacket);
			break;
		case GET_CONTEXT_EXPOSURE:
			get_exposure_time(picam, repacket);
			break;
		default:
			break;
	}
}

void raspicam_set::set_picam_param(std::weak_ptr<raspicam::RaspiCam_Cv> picam, uint8_t command, uint8_t value, xpacket *repacket) {
	switch (command) {
		case SET_CONTEXT_AUTOEXPOSURE:
			set_auto_exposure_time(picam, value, repacket);
			break;
		case GET_CONTEXT_AUTOEXPOSURE:
			get_auto_exposure_time(picam, repacket);
			break;
		default:
			break;
	}
}

void raspicam_set::set_gain_param(std::weak_ptr<raspicam::RaspiCam_Cv> picam, double gain, xpacket *repacket) {
	if (gain <= 0) {
		gain = 1;
	}

	if (gain >= 100) {
		gain = 100;
	}

	std::shared_ptr<raspicam::RaspiCam_Cv> pc = picam.lock();
	bool res = pc->set(CV_CAP_PROP_GAIN, gain);
	if (res) {
		g_data::log(INFO_LOG_LEVEL, "[raspicam_set] Set gain[%f] success.", gain);
	} else {
		g_data::log(ERROR_LOG_LEVEL, "[raspicam_set] Set gain[%f] fail.", gain);
	}
	make_response(SET_CONTEXT_GAIN, gain, repacket);
}

void raspicam_set::get_gain_param(std::weak_ptr<raspicam::RaspiCam_Cv> picam, xpacket *repacket) {
	std::shared_ptr<raspicam::RaspiCam_Cv> pc = picam.lock();
	double gain = pc->get(CV_CAP_PROP_GAIN);
	g_data::log(ERROR_LOG_LEVEL, "[raspicam_set] Get gain[%f].", gain);
	make_response(GET_CONTEXT_GAIN, gain, repacket);
}

void raspicam_set::set_exposure_time(std::weak_ptr<raspicam::RaspiCam_Cv> picam, double exposure_time, xpacket *repacket) {
	if (exposure_time <= 0) {
		exposure_time = 1;
	}

	if (exposure_time >= 100) {
		exposure_time = 100;
	}

	std::shared_ptr<raspicam::RaspiCam_Cv> pc = picam.lock();
	bool res = pc->set(CV_CAP_PROP_EXPOSURE, exposure_time);
	if (res) {
		g_data::log(INFO_LOG_LEVEL, "[raspicam_set] Set Exposure Time[%f] success.", exposure_time);
	} else {
		g_data::log(ERROR_LOG_LEVEL, "[raspicam_set] Set Exposure Time[%f] fail.", exposure_time);
	}
	make_response(SET_CONTEXT_EXPOSURE, exposure_time, repacket);
}

void raspicam_set::get_exposure_time(std::weak_ptr<raspicam::RaspiCam_Cv> picam, xpacket *repacket) {
	std::shared_ptr<raspicam::RaspiCam_Cv> pc = picam.lock();
	double exposure_time = pc->get(CV_CAP_PROP_EXPOSURE);
	make_response(GET_CONTEXT_EXPOSURE, exposure_time, repacket);
}

void raspicam_set::set_auto_exposure_time(std::weak_ptr<raspicam::RaspiCam_Cv> picam, uint8_t auto_set, xpacket *repacket) {
	double value = 50.0;
	if (auto_set != 0) {
		value = -1.0;
	}

	std::shared_ptr<raspicam::RaspiCam_Cv> pc = picam.lock();
	bool res = pc->set(CV_CAP_PROP_EXPOSURE, value);
	if (res) {
		g_data::log(INFO_LOG_LEVEL, "[raspicam_set] Set Auto Exposure Time[%u] success.", auto_set);
	} else {
		g_data::log(ERROR_LOG_LEVEL, "[raspicam_set] Set Auto Exposure Time[%u] fail.", auto_set);
	}
	make_response(SET_CONTEXT_AUTOEXPOSURE, auto_set, repacket);
}

void raspicam_set::get_auto_exposure_time(std::weak_ptr<raspicam::RaspiCam_Cv> picam, xpacket *repacket) {
	std::shared_ptr<raspicam::RaspiCam_Cv> pc = picam.lock();
	double exposure_time = pc->get(CV_CAP_PROP_EXPOSURE);

	uint8_t auto_set = 0;
	if (exposure_time == -1) {
		auto_set = 1;
	}

	g_data::log(INFO_LOG_LEVEL, "[raspicam_set] Get Auto Exposure Time[%u]", auto_set);
	make_response(GET_CONTEXT_AUTOEXPOSURE, auto_set, repacket);
}

void raspicam_set::make_response(uint8_t command, double value, xpacket *repacket) {
	repacket->m_header.h_command = OP_CONTEXTCAM_COMMAND;
	repacket->PushByte(command);
	repacket->PushMem((uint8_t *)&value, sizeof(double));
}

void raspicam_set::make_response(uint8_t command, uint8_t value, xpacket *repacket) {
	repacket->m_header.h_command = OP_CONTEXTCAM_COMMAND;
	repacket->PushByte(command);
	repacket->PushByte(value);
}