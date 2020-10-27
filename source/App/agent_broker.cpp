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