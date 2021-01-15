#include "agent_broker.h"

agent_broker::agent_broker() {
}

agent_broker::~agent_broker() {

}

void agent_broker::set_picam(std::weak_ptr<raspicam::RaspiCam_Cv> picam) {
	picam_ = picam;
}

std::weak_ptr<raspicam::RaspiCam_Cv> agent_broker::picam() {
	return picam_;
}

void agent_broker::update_thermogram_index() {
	cur_thermogram_index_++;
}

uint32_t agent_broker::thermogram_index() {
	return cur_thermogram_index_;
}