#include "raspicam_agent.h"

raspicam_agent::raspicam_agent() {
	run_flag_ = true;

	stream_sock_ = std::make_shared<zmq::socket_t>(g_data::context(), zmq::socket_type::push);
	stream_sock_->connect(g_data::stream_from_camera_address());

	width_ = 640;
	height_ = 480;
	frame_rate_ = 20;

	picam_ = std::make_shared<raspicam::RaspiCam_Cv>();
	jpg_compressor_ = tjInitCompress();
	jpg_buf_ = tjAlloc(width_ * height_ * 3);

	capture_th_ = std::thread(&raspicam_agent::capture, this);
}

raspicam_agent::~raspicam_agent() {
	run_flag_ = false;
	if (capture_th_.joinable()) {
		capture_th_.join();
	}

	if (picam_->isOpened()) {
		picam_->release();
	}

	tjFree(jpg_buf_);
	tjDestroy(jpg_compressor_);
}

void raspicam_agent::capture() {
	while (run_flag_) {
		if (picam_->isOpened() == false) {
			picam_->set(CV_CAP_PROP_FORMAT, CV_8UC3);
			picam_->set(CV_CAP_PROP_FRAME_WIDTH, width_);
			picam_->set(CV_CAP_PROP_FRAME_HEIGHT, height_);
			picam_->set(CV_CAP_PROP_EXPOSURE, -1);
			picam_->set(CV_CAP_PROP_FPS, frame_rate_);

			if (picam_->open() == false) {
				g_data::log(ERROR_LOG_LEVEL, "[raspicam_agent] PiCamera open fail.");
				std::this_thread::sleep_for(std::chrono::seconds(3));
				continue;
			}
		}

		// todo get thermogram index
		uint32_t cur_thermogram_index = 0;
		if (last_thermogram_index_ == cur_thermogram_index) {
			std::this_thread::sleep_for(std::chrono::milliseconds(5));
			continue;
		}
		last_thermogram_index_ = cur_thermogram_index;
		
		if (picam_->grab() == false) {
			g_data::log(ERROR_LOG_LEVEL, "[raspicam_agent] PiCamera grab fail.");
			picam_->release();
			continue;
		}

		cv::Mat img;
		picam_->retrieve(img);

		unsigned long compress_len = 0;
		int ret = tjCompress2(jpg_compressor_, img.data, img.cols, 0, img.rows, TJPF_RGB, &jpg_buf_, &compress_len, TJSAMP_420, compress_rate_, TJFLAG_NOREALLOC);
		if (ret < 0) {
			g_data::log(ERROR_LOG_LEVEL, "[raspicam_agent] PiCamera image compress error");
			continue;
		}
		
		// send jpg image
		stream_data data;
		data.type = RASPICAM_IMAGE;
		data.index = last_thermogram_index_;
		data.length = compress_len;
		data.data = new uint8_t[data.length];
		std::memcpy(data.data, jpg_buf_, data.length);

		zmq::message_t message(&data, sizeof(stream_data));
		stream_sock_->send(message, zmq::send_flags::dontwait);
	}
}