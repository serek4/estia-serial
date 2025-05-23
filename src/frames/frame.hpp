/*
frame.hpp - Estia R32 heat pump data frame base
Copyright (C) 2025 serek4. All rights reserved.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include <Print.h>
#include <WString.h>
#include <deque>
#include <stdint.h>
#include <utility>
#include <vector>

#define FRAME_BEGIN 0xa0
#define FRAME_TYPE_OFFSET 2
#define FRAME_DATA_LEN_OFFSET 3
#define FRAME_DATA_OFFSET 4
#define FRAME_DATA_TYPE_OFFSET 9
#define FRAME_HEAD_AND_CRC_LEN 6

#define FRAME_TYPE_HEARTBEAT 0x10
#define FRAME_TYPE_SET 0x11
#define FRAME_TYPE_REQ_DATA 0x17
#define FRAME_TYPE_ACK 0x18
#define FRAME_TYPE_RES_DATA 0x1a
#define FRAME_TYPE_UPDATE 0x1c
#define FRAME_TYPE_STATUS 0x58

#define FRAME_DATA_TYPE_HEARTBEAT 0x008a
#define FRAME_DATA_TYPE_STATUS 0x03c6
#define FRAME_DATA_TYPE_MODE_CHANGE 0x03c4
#define FRAME_DATA_TYPE_OPERATION_SWITCH 0x0041
#define FRAME_DATA_TYPE_TEMPERATURE_CHANGE 0x03c1
#define FRAME_DATA_TYPE_FORCE_DEFROST 0x0015
#define FRAME_DATA_TYPE_DATA_REQUEST 0x0080
#define FRAME_DATA_TYPE_DATA_RESPONSE 0x00ef
#define FRAME_DATA_TYPE_ACK 0x00a1
#define FRAME_DATA_TYPE_SHORT_STATUS 0x002b

#define FRAME_MIN_DATA_LEN 0x07
#define FRAME_HEARTBEAT_DATA_LEN 0x07
#define FRAME_SET_MODE_DATA_LEN 0x0b
#define FRAME_SWITCH_DATA_LEN 0x08
#define FRAME_TEMPERATURE_DATA_LEN 0x0c
#define FRAME_REQ_DATA_DATA_LEN 0x0f
#define FRAME_ACK_DATA_LEN 0x09
#define FRAME_RES_DATA_DATA_LEN 0x0d
#define FRAME_STATUS_DATA_LEN 0x19
#define FRAME_UPDATE_DATA_LEN 0x0f
#define FRAME_FORCE_DEFROST_DATA_LEN 0x0a

#define FRAME_MIN_LEN FRAME_HEAD_AND_CRC_LEN + FRAME_MIN_DATA_LEN
#define FRAME_HEARTBEAT_LEN 13
#define FRAME_SET_MODE_LEN 17
#define FRAME_SWITCH_LEN 14
#define FRAME_TEMPERATURE_LEN 18
#define FRAME_REQ_DATA_LEN 21
#define FRAME_ACK_LEN 15
#define FRAME_RES_DATA_LEN 19
#define FRAME_STATUS_LEN 31
#define FRAME_UPDATE_LEN 21
#define FRAME_FORCE_DEFROST_LEN 16

using ReadBuffer = std::deque<uint8_t>;
using FrameBuffer = std::vector<uint8_t>;

class EstiaFrame {
  private:
  protected:
	uint8_t type;
	uint8_t dataLength;
	uint16_t crc;

	void updateDataType();
	uint16_t crc16(uint8_t* data, size_t len);    // CRC-16/MCRF4XX

  public:
	EstiaFrame(FrameBuffer&& buffer, uint8_t length);
	EstiaFrame(FrameBuffer& buffer, uint8_t length);
	EstiaFrame(uint8_t type, uint8_t length);

	uint8_t length;
	FrameBuffer buffer;
	uint16_t dataType;

	bool setByte(uint8_t position, uint8_t value, bool updateCrc = true);
	uint8_t getByte(uint8_t position);
	uint8_t* getBuffer();
	void updateCrc();
	bool insertData(uint8_t* data, bool updateCrc = true);
	static String stringify(EstiaFrame* frame);
	static String stringify(const FrameBuffer& buffer);
	static FrameBuffer readBuffToFrameBuff(const ReadBuffer& buffer);
	static bool isStatusFrame(const ReadBuffer& buffer);
	static bool isStatusUpdateFrame(const ReadBuffer& buffer);
	static bool isAckFrame(const ReadBuffer& buffer);
};
