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

#define FRAME_TYPE_OFFSET 2
#define FRAME_DATA_LEN_OFFSET 3
#define FRAME_DATA_HEADER_OFFSET 4
#define FRAME_SRC_OFFSET 5
#define FRAME_DST_OFFSET 7
#define FRAME_DATA_TYPE_OFFSET 9
#define FRAME_DATA_OFFSET 11

#define FRAME_TYPE_CTRL_FRAME 0x10
#define FRAME_TYPE_CMD 0x11
#define FRAME_TYPE_REQ_DATA 0x17
#define FRAME_TYPE_ACK 0x18
#define FRAME_TYPE_RES_DATA 0x1a
#define FRAME_TYPE_UPDATE 0x1c
#define FRAME_TYPE_STATUS 0x58

#define FRAME_BEGIN 0xa000

#define FRAME_SRC_DST_MASTER 0x0800
#define FRAME_SRC_DST_REMOTE 0x0040
#define FRAME_SRC_DST_BROADCAST 0x00fe

#define FRAME_DATA_TYPE_HEARTBEAT 0x008a
#define FRAME_DATA_TYPE_STATUS 0x03c6
#define FRAME_DATA_TYPE_MODE_CHANGE 0x03c4
#define FRAME_DATA_TYPE_OPERATION_MODE 0x03c0
#define FRAME_DATA_TYPE_OPERATION_SWITCH 0x0041
#define FRAME_DATA_TYPE_TEMPERATURE_CHANGE 0x03c1
#define FRAME_DATA_TYPE_SPECIAL_CMD 0x0015
#define FRAME_DATA_TYPE_DATA_REQUEST 0x0080
#define FRAME_DATA_TYPE_DATA_RESPONSE 0x00ef
#define FRAME_DATA_TYPE_ACK 0x00a1
#define FRAME_DATA_TYPE_SHORT_STATUS 0x002b

#define FRAME_MIN_DATA_LEN 0x07
#define FRAME_DATA_HEADER_LEN 0x07
#define FRAME_HEARTBEAT_DATA_LEN 0x07
#define FRAME_SET_MODE_DATA_LEN 0x0b
#define FRAME_OPERATION_MODE_DATA_LEN 0x08
#define FRAME_SWITCH_DATA_LEN 0x08
#define FRAME_TEMPERATURE_DATA_LEN 0x0c
#define FRAME_REQ_DATA_DATA_LEN 0x0f
#define FRAME_ACK_DATA_LEN 0x09
#define FRAME_RES_DATA_DATA_LEN 0x0d
#define FRAME_STATUS_DATA_LEN 0x19
#define FRAME_UPDATE_DATA_LEN 0x0f
#define FRAME_FORCE_DEFROST_DATA_LEN 0x0a

#define FRAME_HEAD_LEN 0x04
#define FRAME_CRC_LEN 0x02
#define FRAME_HEAD_AND_CRC_LEN (FRAME_HEAD_LEN + FRAME_CRC_LEN)
#define FRAME_MIN_LEN (FRAME_HEAD_LEN + FRAME_MIN_DATA_LEN + FRAME_CRC_LEN)
#define FRAME_MAX_LEN 45
#define FRAME_HEARTBEAT_LEN 13
#define FRAME_SET_MODE_LEN 17
#define FRAME_OPERATION_MODE_LEN 14
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
	FrameBuffer buffer;
	uint8_t length;
	uint8_t type;
	uint8_t dataLength;
	uint16_t src;
	uint16_t dst;
	uint16_t dataType;
	uint16_t crc;

	void setSrc(uint16_t src, bool updateCrc = false);
	void setDst(uint16_t dst, bool updateCrc = false);
	void setDataType(uint16_t dataType, bool updateCrc = false);
	bool insertData(uint8_t* data, bool incHeader = true, bool updateCrc = false);
	bool setByte(uint8_t offset, uint8_t value, bool updateCrc = false);
	bool writeUint16(uint8_t offset, uint16_t data);
	uint16_t readUint16(uint8_t offset);
	uint8_t checkFrame(uint8_t type, uint16_t dataType);
	void updateCrc();

  public:
	enum Error {
		err_ok,
		err_crc,
		err_frame_type,
		err_data_len,
		err_data_type,
		err_other,
	};

	EstiaFrame(FrameBuffer&& buffer, uint8_t length);
	EstiaFrame(FrameBuffer& buffer, uint8_t length);
	EstiaFrame(uint8_t type, uint8_t length);

	const uint8_t* data() const;
	uint8_t size() const;
	template <typename Buffer>
	static bool writeUint16(Buffer& buffer, uint8_t offset, uint16_t data);
	template <typename Buffer>
	static uint16_t readUint16(const Buffer& buffer, uint8_t offset);
	static uint16_t crc16(uint8_t* data, size_t len);    // CRC-16/MCRF4XX
	String stringify();
	template <typename Buffer>
	static String stringify(const Buffer& buffer);
	static FrameBuffer readBuffToFrameBuff(const ReadBuffer& buffer);
	template <typename Buffer>
	static bool isStatusFrame(const Buffer& buffer);
	template <typename Buffer>
	static bool isStatusUpdateFrame(const Buffer& buffer);
	template <typename Buffer>
	static bool isAckFrame(const Buffer& buffer);
	template <typename Buffer>
	static bool isDataResFrame(const Buffer& buffer);

	friend class EstiaSerial;
};
