#pragma once

#include "global.hpp"
#include <raspicam/raspicam_cv.h>

class raspicam_set {
public:
	static void proc_command(std::weak_ptr<raspicam::RaspiCam_Cv>, xpacket *, xpacket *);
	static void set_picam_param(std::weak_ptr<raspicam::RaspiCam_Cv>, uint8_t, double, xpacket *);
	static void set_picam_param(std::weak_ptr<raspicam::RaspiCam_Cv>, uint8_t, uint8_t, xpacket *);
	static void set_gain_param(std::weak_ptr<raspicam::RaspiCam_Cv>, double, xpacket *);
	static void get_gain_param(std::weak_ptr<raspicam::RaspiCam_Cv>, xpacket *);
	static void set_exposure_time(std::weak_ptr<raspicam::RaspiCam_Cv>, double, xpacket *);
	static void get_exposure_time(std::weak_ptr<raspicam::RaspiCam_Cv>, xpacket *);
	static void set_auto_exposure_time(std::weak_ptr<raspicam::RaspiCam_Cv>, uint8_t, xpacket *);
	static void get_auto_exposure_time(std::weak_ptr<raspicam::RaspiCam_Cv>, xpacket *);
	static void make_response(uint8_t, double, xpacket *);
	static void make_response(uint8_t, uint8_t, xpacket *);
};