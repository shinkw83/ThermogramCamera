#include "xpacket.h"
#include <cstring>

xpacket::xpacket()
{
	m_alloc_mode = ALLOC_DEFAULT;
	m_buf = new uint8_t[DEFAULT_ALLOC_SIZE];
	m_buf_size = DEFAULT_ALLOC_SIZE;
	m_body_size = m_buf_size - XPACKET_HEADER_SIZE - XPACKET_TAIL_SIZE;
	m_cur_pos = XPACKET_HEADER_SIZE;
}

xpacket::xpacket(size_t alloc_size)
{
	m_alloc_mode = ALLOC_USER_SIZE;
	m_buf_size = alloc_size;

	if(m_buf_size < XPACKET_HEADER_SIZE + XPACKET_TAIL_SIZE)
		alloc_size = DEFAULT_ALLOC_SIZE;

	m_buf = new uint8_t[alloc_size];
	m_body_size = m_buf_size - XPACKET_HEADER_SIZE - XPACKET_TAIL_SIZE;
	m_cur_pos = XPACKET_HEADER_SIZE;
}

xpacket::xpacket(uint8_t* alloc_buf, size_t alloc_size, bool headertail_included)
{
	m_alloc_mode = ALLOC_USER_BUF;
	m_buf = alloc_buf;
	m_buf_size = alloc_size;
	m_body_size = alloc_size - XPACKET_HEADER_SIZE - XPACKET_TAIL_SIZE;

	if (headertail_included == true)
		memcpy(&m_header, alloc_buf, XPACKET_HEADER_SIZE);
	
	m_cur_pos = XPACKET_HEADER_SIZE;
}

xpacket::xpacket(xpacket* a)
{
	m_alloc_mode = ALLOC_USER_SIZE;
	m_buf_size = m_total_pack_size = a->m_total_pack_size;
	m_buf = new uint8_t[m_total_pack_size];
	memcpy(&m_header, a->m_buf, XPACKET_HEADER_SIZE);
	memcpy(m_buf, a->m_buf, m_total_pack_size);
}

xpacket::~xpacket()
{
	if (m_alloc_mode != ALLOC_USER_BUF)
		delete[] m_buf;
}


void xpacket::Split(xpacket *org_packet, vector<xpacket*>& slices, size_t split_size)
{
	size_t pos = 0;
	size_t total_body_size = org_packet->GetTotalPackSize() - xpacket::XPACKET_HEADER_SIZE - xpacket::XPACKET_TAIL_SIZE;
	size_t split_body_size = split_size - xpacket::XPACKET_HEADER_SIZE - xpacket::XPACKET_TAIL_SIZE;
	uint16_t total_slices = uint16_t(total_body_size % split_body_size == 0 ? total_body_size/ split_body_size : (total_body_size / split_body_size) + 1);

	for (uint16_t i = 0; i < total_slices; i++)
	{
		xpacket* p = new xpacket(split_size);

		p->m_header = org_packet->m_header;
		p->m_header.h_total_sub = total_slices;
		p->m_header.h_sub_index = i;

		int cp_len = min(int(split_body_size), (int(total_body_size) - int(split_body_size*i)));
		p->PushMem(org_packet->m_buf + xpacket::XPACKET_HEADER_SIZE + pos, cp_len);
		p->Pack();

		
		slices.push_back(p);

		pos += cp_len;
	}
}

xpacket* xpacket::Merge(vector<xpacket*>* slices)
{
	size_t total_body_size = 0;
	for (int i = 0; i < (int)slices->size(); i++)
	{
		total_body_size += ((*slices)[i]->m_header.h_packet_size - XPACKET_HEADER_SIZE - XPACKET_TAIL_SIZE);
	}
	xpacket* merged_packet = new xpacket(total_body_size + XPACKET_HEADER_SIZE + XPACKET_TAIL_SIZE);
	merged_packet->m_header = slices->front()->m_header;

	for (int i = 0; i < (int)slices->size(); i++)
	{
		size_t cp_len = ((*slices)[i]->m_header.h_packet_size - XPACKET_HEADER_SIZE - XPACKET_TAIL_SIZE);
		merged_packet->PushMem((*slices)[i]->GetBuf() + XPACKET_HEADER_SIZE, cp_len);
	}

	merged_packet->Pack();
	return merged_packet;
}

int xpacket::Validate()
{
	if (m_buf_size != m_header.h_packet_size)
		return -1;
	xpacket_tail normal_tail;
	if (m_buf[m_buf_size - 2] != normal_tail.h_prefix[0] || m_buf[m_buf_size - 1] != normal_tail.h_prefix[1])
		return -2;
	
	return 0;
}

int xpacket::PushByte(const uint8_t data)
{
	if (m_cur_pos + 1 > m_buf_size - XPACKET_TAIL_SIZE)
		return -1;
	m_buf[m_cur_pos++] = data;
	return 0;
}

int xpacket::PushWord(const uint16_t data)
{
	if (m_cur_pos + 2 > m_buf_size - XPACKET_TAIL_SIZE)
		return -1;
	memcpy(m_buf+m_cur_pos, &data, 2);
	m_cur_pos += 2;
	return 0;
}

int xpacket::PushDWord(const uint32_t data)
{
	if (m_cur_pos + 4 > m_buf_size - XPACKET_TAIL_SIZE)
		return -1;
	memcpy(m_buf + m_cur_pos, &data, 4);
	m_cur_pos += 4;
	return 0;
}

int xpacket::PushShort(const short data)
{
    if (m_cur_pos + 2 > m_buf_size - XPACKET_TAIL_SIZE)
        return -1;
    memcpy(m_buf + m_cur_pos, &data, 2);
    m_cur_pos += 2;
    return 0;
}

int xpacket::PushMem(uint8_t* data_p, size_t len)
{	
	if (m_cur_pos + len > m_buf_size - XPACKET_TAIL_SIZE)
	{
        return -1;
	}	

	memcpy(m_buf + m_cur_pos, data_p, len);
	m_cur_pos += len;
    
	return 0;
}

size_t xpacket::Pack()
{
	size_t total_len = m_cur_pos + XPACKET_TAIL_SIZE;
	m_header.h_packet_size = (uint32_t)total_len;
	memcpy(m_buf, &m_header, XPACKET_HEADER_SIZE);
	memcpy(m_buf + m_cur_pos, &m_tail, XPACKET_TAIL_SIZE);
		
	m_total_pack_size = total_len;
	return total_len;
}

size_t xpacket::Pack(uint8_t** buf_p)
{
	size_t total_len = Pack();

	if(*buf_p == NULL)
		*buf_p = new uint8_t[total_len];

	memcpy(*buf_p, m_buf, total_len);
	
	m_total_pack_size = total_len;
	return total_len;
}

int xpacket::PopByte(uint8_t& data)
{
	if (m_cur_pos + 1 > m_buf_size - XPACKET_TAIL_SIZE)
		return -1;
	data = m_buf[m_cur_pos];
	++m_cur_pos;
	return 0;
}

int xpacket::PopWord(uint16_t& data)
{
	if (m_cur_pos + 2 > m_buf_size - XPACKET_TAIL_SIZE)
		return -1;
	memcpy(&data, m_buf + m_cur_pos, 2);
	m_cur_pos += 2;
	return 0;
}

int xpacket::PopDWord(uint32_t& data)
{
	if (m_cur_pos + 4 > m_buf_size - XPACKET_TAIL_SIZE)
		return -1;
	memcpy(&data, m_buf + m_cur_pos, 4);
	m_cur_pos += 4;
	return 0;
}

int xpacket::PopMem(uint8_t* data_p, const size_t len)
{
	if (m_cur_pos + len > m_buf_size - XPACKET_TAIL_SIZE)
		return -1;

	memcpy(data_p, m_buf + m_cur_pos, len);
	m_cur_pos += len;
	return 0;
}

int xpacket::PopShort(short& data)
{
    if (m_cur_pos + 2 > m_buf_size - XPACKET_TAIL_SIZE)
        return -1;
    memcpy(&data, m_buf + m_cur_pos, 2);
    m_cur_pos += 2;
    return 0;
}

void xpacket::ResetPos()
{
	m_cur_pos = XPACKET_HEADER_SIZE;
}
