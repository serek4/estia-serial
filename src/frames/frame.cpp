/*
frame.cpp - Estia R32 heat pump data frame base
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

#include "frame.hpp"

// frame from buffer (rvalue)
EstiaFrame::EstiaFrame(FrameBuffer&& buffer, uint8_t length)
    : length(length)
    , buffer(buffer)
    , type(0x00)
    , dataLength(0x00)
    , crc(0x0000) {
	if (length < FRAME_MIN_LEN) {    // ensure minimum buffer size
		this->length = FRAME_MIN_LEN;
	}
	this->buffer.resize(this->length, 0x00);    // resize to proper length
	type = this->buffer.at(FRAME_TYPE_OFFSET);
	dataLength = this->buffer.at(FRAME_DATA_LEN_OFFSET);
	crc = (this->buffer.at(this->length - 2) << 8) | this->buffer.at(this->length - 1);
}

// frame from buffer (lvalue)
EstiaFrame::EstiaFrame(FrameBuffer& buffer, uint8_t length)
    : EstiaFrame::EstiaFrame(std::forward<FrameBuffer>(buffer), length) {
}

// frame with type, empty data and no crc
EstiaFrame::EstiaFrame(uint8_t type, uint8_t length)
    : length(length)
    , buffer(length, 0x00)
    , type(type)
    , dataLength(length - FRAME_HEAD_AND_CRC_LEN)
    , crc(0x0000) {
	if (length < FRAME_MIN_LEN) {    // ensure minimum buffer size
		length = FRAME_MIN_LEN;
		buffer.resize(FRAME_MIN_LEN, 0x00);
	}
	buffer.front() = FRAME_BEGIN;
	buffer.at(FRAME_TYPE_OFFSET) = type;
	buffer.at(FRAME_DATA_LEN_OFFSET) = dataLength;
}

bool EstiaFrame::setByte(uint8_t position, uint8_t value, bool updateCrc) {
	if (position > length) { return false; }
	buffer.at(position) = value;
	if (updateCrc) { this->updateCrc(); }
	return true;
}

uint8_t EstiaFrame::getByte(uint8_t position) {
	return buffer.at(position);
}

uint8_t* EstiaFrame::getBuffer() {
	return buffer.data();
}

void EstiaFrame::updateCrc() {
	crc = crc16(buffer.data(), length - 2);
	buffer.at(length - 2) = (crc & 0xff00) >> 8;
	buffer.at(length - 1) = crc & 0x00ff;
}

bool EstiaFrame::insertData(uint8_t* data, bool updateCrc) {
	for (uint8_t idx = 0; idx < dataLength; idx++) {
		this->buffer.at(idx + FRAME_DATA_OFFSET) = data[idx];
	}
	if (updateCrc) { this->updateCrc(); }
	return true;
}

String EstiaFrame::stringify(EstiaFrame* frame) {
	return stringify(frame->buffer);
}

String EstiaFrame::stringify(const FrameBuffer& buffer) {
	String stringifyBuffer = "";
	for (auto& byte : buffer) {
		if (byte < 0x10) { stringifyBuffer += "0"; }
		stringifyBuffer += String(byte, HEX) + " ";
	}
	stringifyBuffer.trim();
	return stringifyBuffer;
}

// https://gist.github.com/aurelj/270bb8af82f65fa645c1?permalink_comment_id=2884584#gistcomment-2884584
uint16_t EstiaFrame::crc16(uint8_t* data, size_t len) {
	uint16_t crc = 0xffff;
	uint8_t L;
	uint8_t t;
	if (!data || len <= 0) { return crc; }
	while (len--) {
		crc ^= *data++;
		L = crc ^ (crc << 4);
		t = (L << 3) | (L >> 5);
		L ^= (t & 0x07);
		t = (t & 0xf8) ^ (((t << 1) | (t >> 7)) & 0x0f) ^ (uint8_t)(crc >> 8);
		crc = (L << 8) | t;
	}
	return crc;
}

FrameBuffer EstiaFrame::readBuffToFrameBuff(const ReadBuffer& buffer) {
	FrameBuffer frameBuffer;
	for (auto& byte : buffer) {
		frameBuffer.push_back(byte);
	}
	return frameBuffer;
}

bool EstiaFrame::isStatusFrame(const ReadBuffer& buffer) {
	return buffer.size() == FRAME_STATUS_LEN
	       && buffer.at(FRAME_TYPE_OFFSET) == FRAME_TYPE_STATUS
	       && buffer.at(FRAME_DATA_LEN_OFFSET) == FRAME_STATUS_DATA_LEN;
}

bool EstiaFrame::isStatusUpdateFrame(const ReadBuffer& buffer) {
	return buffer.size() == FRAME_UPDATE_LEN
	       && buffer.at(FRAME_TYPE_OFFSET) == FRAME_TYPE_UPDATE
	       && buffer.at(FRAME_DATA_LEN_OFFSET) == FRAME_UPDATE_DATA_LEN;
}
