#include "lepton_agent.h"
#include "Palettes.h"
#include "SPI.h"
#include "Lepton_I2C.h"

lepton_agent::lepton_agent(agent_broker *broker) {
	broker_ = broker;
	run_flag_ = true;

	colormap_ = 3;		// 1 : colormap_rainbow, 2 : colormap_grayscale, 3 : colormap_ironblack(deafault)
	selected_colormap_ = colormap_ironblack;
	selected_colormap_size_ = get_size_colormap_ironblack();

	stream_sock_ = std::make_shared<zmq::socket_t>(g_data::context(), zmq::socket_type::push);
	stream_sock_->connect(g_data::stream_from_camera_address());

	width_ = 160;
	height_ = 120;

	jpg_compressor_ = tjInitCompress();
	jpg_buf_ = tjAlloc(width_ * height_ * 3);

	compress_rate_ = 75;

	run_th_ = std::thread(&lepton_agent::run, this);
}

lepton_agent::~lepton_agent() {
	run_flag_ = false;
	if (run_th_.joinable()) {
		run_th_.join();
	}

	tjFree(jpg_buf_);
	tjDestroy(jpg_compressor_);
}

void lepton_agent::set_colormap(int colormap) {
	switch (colormap) {
	case 1:
		colormap_ = 1;
		selected_colormap_ = colormap_rainbow;
		selected_colormap_size_ = get_size_colormap_rainbow();
		break;
	case 2:
		colormap_ = 2;
		selected_colormap_ = colormap_grayscale;
		selected_colormap_size_ = get_size_colormap_grayscale();
		break;
	default:
		colormap_ = 3;
		selected_colormap_ = colormap_ironblack;
		selected_colormap_size_ = get_size_colormap_ironblack();
		break;
	}
}

void lepton_agent::run() {
	float diff = range_max_ - range_min_;
	float scale = 255/diff;
	uint16_t n_wrong_segment = 0;
	uint16_t n_zero_value_drop_frame = 0;
	uint16_t maxValue = range_max_;
	uint16_t minValue = range_min_;

	SpiOpenPort(0, SPI_SPEED);

	while (run_flag_) {
		//read data packets from lepton over SPI
		int resets = 0;
		int segmentNumber = -1;
		for(int j=0;j<PACKETS_PER_FRAME;j++) {
			//if it's a drop packet, reset j to 0, set to -1 so he'll be at 0 again loop
			read(spi_cs0_fd, result_+sizeof(uint8_t)*PACKET_SIZE*j, sizeof(uint8_t)*PACKET_SIZE);
			int packetNumber = result_[j*PACKET_SIZE+1];
			if(packetNumber != j) {
				j = -1;
				resets += 1;
				usleep(1000);
				//Note: we've selected 750 resets as an arbitrary limit, since there should never be 750 "null" packets between two valid transmissions at the current poll rate
				//By polling faster, developers may easily exceed this count, and the down period between frames may then be flagged as a loss of sync
				if(resets == 750) {
					SpiClosePort(0);
					lepton_reboot();
					n_wrong_segment = 0;
					n_zero_value_drop_frame = 0;
					usleep(750000);
					SpiOpenPort(0, SPI_SPEED);
				}
				continue;
			}
			if (packetNumber == 20) {
				segmentNumber = (result_[j*PACKET_SIZE] >> 4) & 0x0f;
				if ((segmentNumber < 1) || (4 < segmentNumber)) {
					g_data::log(ERROR_LOG_LEVEL, "[lepton_agent] Wrong segment number %d", segmentNumber);
					break;
				}
			}
		}

		int iSegmentStart = 1;
		int iSegmentStop;

		if ((segmentNumber < 1) || (4 < segmentNumber)) {
			n_wrong_segment++;
			if ((n_wrong_segment % 12) == 0) {
				g_data::log(WARN_LOG_LEVEL, "[lepton_agent] Got wrong segment number continuously %u times", n_wrong_segment);
			}
			continue;
		}
		if (n_wrong_segment != 0) {
			g_data::log(WARN_LOG_LEVEL, "[lepton_agent] Got wrong segment number continuously %u times [RECOVERED] %d", n_wrong_segment, segmentNumber);
			n_wrong_segment = 0;
		}

		//
		memcpy(shelf_[segmentNumber - 1], result_, sizeof(uint8_t) * PACKET_SIZE*PACKETS_PER_FRAME);
		if (segmentNumber != 4) {
			continue;
		}
		iSegmentStop = 4;

		maxValue = 0;
		minValue = 65535;
		for(int iSegment = iSegmentStart; iSegment <= iSegmentStop; iSegment++) {
			for(int i=0;i<FRAME_SIZE_UINT16;i++) {
				//skip the first 2 uint16_t's of every packet, they're 4 header bytes
				if(i % PACKET_SIZE_UINT16 < 2) {
					continue;
				}

				//flip the MSB and LSB at the last second
				uint16_t value = (shelf_[iSegment - 1][i*2] << 8) + shelf_[iSegment - 1][i*2+1];
				if (value == 0) {
					// Why this value is 0?
					continue;
				}
				if (value > maxValue) {
					maxValue = value;
				}
				if (value < minValue) {
					minValue = value;
				}
			}
		}
		diff = maxValue - minValue;
		scale = 255/diff;

		int row, column;
		uint16_t value;
		uint16_t valueFrameBuffer;
		cv::Mat image(height_, width_, CV_8UC3);
		for(int iSegment = iSegmentStart; iSegment <= iSegmentStop; iSegment++) {
			int ofsRow = 30 * (iSegment - 1);
			for(int i=0;i<FRAME_SIZE_UINT16;i++) {
				//skip the first 2 uint16_t's of every packet, they're 4 header bytes
				if(i % PACKET_SIZE_UINT16 < 2) {
					continue;
				}

				//flip the MSB and LSB at the last second
				valueFrameBuffer = (shelf_[iSegment - 1][i*2] << 8) + shelf_[iSegment - 1][i*2+1];
				if (valueFrameBuffer == 0) {
					// Why this value is 0?
					n_zero_value_drop_frame++;
					if ((n_zero_value_drop_frame % 12) == 0) {
						g_data::log(WARN_LOG_LEVEL, "[lepton_agent] Found zero-value. Drop the frame continuously %u times", n_zero_value_drop_frame);
					}
					break;
				}

				//
				value = (valueFrameBuffer - minValue) * scale;
				int ofs_r = 3 * value + 0; if (selected_colormap_size_ <= ofs_r) ofs_r = selected_colormap_size_ - 1;
				int ofs_g = 3 * value + 1; if (selected_colormap_size_ <= ofs_g) ofs_g = selected_colormap_size_ - 1;
				int ofs_b = 3 * value + 2; if (selected_colormap_size_ <= ofs_b) ofs_b = selected_colormap_size_ - 1;
				cv::Vec3b v;
				v.val[0] = selected_colormap_[ofs_b];
				v.val[1] = selected_colormap_[ofs_g];
				v.val[2] = selected_colormap_[ofs_r];
				column = (i % PACKET_SIZE_UINT16) - 2 + (width_ / 2) * ((i % (PACKET_SIZE_UINT16 * 2)) / PACKET_SIZE_UINT16);
				row = i / PACKET_SIZE_UINT16 / 2 + ofsRow;
				image.at<cv::Vec3b>(row, column) = v;
			}
		}

		if (n_zero_value_drop_frame != 0) {
			g_data::log(WARN_LOG_LEVEL, "[lepton_agent] Found zero-value. Drop the frame continuously %u times [RECOVERED]", n_zero_value_drop_frame);
			n_zero_value_drop_frame = 0;
		}

		// stream image to jpeg
		unsigned long compress_len = 0;
		int ret = tjCompress2(jpg_compressor_, image.data, image.cols, 0, image.rows, TJPF_RGB, &jpg_buf_, &compress_len, TJSAMP_420, compress_rate_, TJFLAG_NOREALLOC);
		if (ret < 0) {
			g_data::log(ERROR_LOG_LEVEL, "[lepton_agent] LEPTON image compress error");
			continue;
		}

		// send jpg image
		stream_data data;
		data.type = THERMOGRAM_IMAGE;
		data.index = 0;
		data.length = compress_len;
		data.data = new uint8_t[data.length];
		std::memcpy(data.data, jpg_buf_, data.length);

		zmq::message_t message(&data, sizeof(stream_data));
		stream_sock_->send(message, zmq::send_flags::dontwait);

		broker_->update_thermogram_index();
	}

	SpiClosePort(0);
}