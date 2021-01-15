#pragma once

#include "global.hpp"
#include "agent_broker.h"
#include <turbojpeg.h>
#include <stdint.h>
#include <ctime>
#include <opencv2/opencv.hpp>

#define PACKET_SIZE 164
#define PACKET_SIZE_UINT16 (PACKET_SIZE/2)
#define PACKETS_PER_FRAME 60
#define FRAME_SIZE_UINT16 (PACKET_SIZE_UINT16*PACKETS_PER_FRAME)
#define SPI_SPEED		20 * 1000 * 1000		// SPI bus speed 20MHz

class lepton_agent {
public:
	lepton_agent(agent_broker *broker);
	~lepton_agent();

	void set_colormap(int colormap);

private:
	void run();

private:
	std::shared_ptr<zmq::socket_t> stream_sock_;

	tjhandle jpg_compressor_;
	uint8_t *jpg_buf_;

	std::thread run_th_;
	std::atomic<bool> run_flag_;

	int width_;
	int height_;
	int compress_rate_;

	agent_broker *broker_;

	int colormap_;
	const int *selected_colormap_;
	int selected_colormap_size_;

	uint16_t range_min_ = 30000;
	uint16_t range_max_ = 32000;

	uint8_t result_[PACKET_SIZE*PACKETS_PER_FRAME];
	uint8_t shelf_[4][PACKET_SIZE*PACKETS_PER_FRAME];
};