#pragma once

#include "global.hpp"
#include <raspicam/raspicam_cv.h>

class agent_broker {
public:
	agent_broker();
	~agent_broker();

	void set_picam(std::weak_ptr<raspicam::RaspiCam_Cv> picam);
	std::weak_ptr<raspicam::RaspiCam_Cv> picam();

	void update_thermogram_index();
	uint32_t thermogram_index();

private:

private:
	std::weak_ptr<raspicam::RaspiCam_Cv> picam_;

	uint32_t cur_thermogram_index_ = 0;
};