#pragma once

#include "global.hpp"
#include <raspicam/raspicam_cv.h>

class agent_broker {
public:
	agent_broker();
	~agent_broker();

	void set_picam(std::weak_ptr<raspicam::RaspiCam_Cv> picam);
	std::weak_ptr<raspicam::RaspiCam_Cv> picam();

private:

private:
	std::weak_ptr<raspicam::RaspiCam_Cv> picam_;
};