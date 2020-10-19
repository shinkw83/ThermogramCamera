#pragma once

#include <cstdlib>
#include <iostream>
#include <cstddef>
#include <cstdint>
#include <vector>
#include <mutex>
#include <utility>

using namespace std;

#pragma pack(push,1)

struct xpacket_header
{
	uint8_t h_prefix[4] = { 'R','X','M','G' };
	uint8_t h_protocol_version = 0;
	uint32_t h_packet_size;
	uint8_t h_command;
	uint8_t h_command_type;
	uint8_t h_command_target;
	uint32_t h_index;
	uint16_t h_sub_index = 0;
	uint16_t h_total_sub = 0;
	uint8_t h_request_result;
	uint8_t h_reserve1;
	uint8_t h_reserve2;
};

struct xpacket_tail
{
	uint8_t h_prefix[2] = { 0xFE, 0xFF };
};

#pragma pack(pop)

struct mem_mark
{
	size_t pos;
	uint8_t* buf;
	size_t len;

	mem_mark() {}
	mem_mark(size_t _pos, uint8_t* _buf, size_t _len)
	{
		pos = _pos;
		buf = _buf;
		len = _len;
	}
};

class xpacket
{
public:
	enum {ALLOC_DEFAULT = 0, ALLOC_USER_SIZE, ALLOC_USER_BUF};
	const static size_t PREFIX_SIZE = 4;
	const static size_t XPACKET_HEADER_SIZE = sizeof(xpacket_header); //23
	const static size_t XPACKET_TAIL_SIZE = sizeof(xpacket_tail); //2
	const static size_t DEFAULT_ALLOC_SIZE = 64;

	xpacket();
	xpacket(size_t alloc_size);
	xpacket(uint8_t* alloc_buf, size_t alloc_size, bool headertail_included);
	xpacket(xpacket* a);

	~xpacket();

	static void Split(xpacket *org_packet, vector<xpacket*>& slices, size_t split_size);
	static xpacket* Merge(vector<xpacket*>* slices);

	int Validate();

	xpacket_header m_header;
	xpacket_tail m_tail;
	
	size_t Pack();
	size_t Pack(uint8_t** buf_p);

	int PushByte(const uint8_t data);
	int PushWord(const uint16_t data);
	int PushDWord(const uint32_t data);
	int PushMem(uint8_t* data_p, size_t len);
    int PushShort(const short data);
    
    int PopShort(short& data);
    int PopByte(uint8_t& data);
	int PopWord(uint16_t& data);
	int PopDWord(uint32_t& data);
	int PopMem(uint8_t* data_p, const size_t len);

	uint8_t* GetBuf() { return m_buf; }
	size_t GetTotalPackSize() { return m_total_pack_size; }

	void ResetPos();
	
private:
	uint8_t m_alloc_mode;
	uint8_t* m_buf;
	size_t m_buf_size;
	size_t m_body_size;
	size_t m_cur_pos;
	size_t m_total_pack_size = 0;
};

