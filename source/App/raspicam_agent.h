#pragma once

#include "global.hpp"
#include <raspicam/raspicam_cv.h>
#include <turbojpeg.h>

class raspicam_agent {
public:
	raspicam_agent();
	~raspicam_agent();

private:
	void capture();

private:
	std::shared_ptr<zmq::socket_t> stream_sock_;
	std::shared_ptr<raspicam::RaspiCam_Cv> picam_;

	tjhandle jpg_compressor_;
	uint8_t *jpg_buf_;

	uint32_t last_thermogram_index_;

	std::thread capture_th_;
	std::atomic<bool> run_flag_;

	int width_;
	int height_;
	int frame_rate_;
	int compress_rate_;
};