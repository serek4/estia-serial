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
    , src(0x0000)
    , dst(0x0000)
    , dataType(0x0000)
    , crc(0x0000) {
	if (length < FRAME_MIN_LEN) {    // ensure minimum buffer size
		this->length = FRAME_MIN_LEN;
	}
	this->buffer.resize(this->length, 0x00);    // resize to proper length
	type = this->buffer.at(FRAME_TYPE_OFFSET);
	dataLength = this->buffer.at(FRAME_DATA_LEN_OFFSET);
	src = readUint16(FRAME_SRC_OFFSET);
	dst = readUint16(FRAME_DST_OFFSET);
	dataType = readUint16(FRAME_DATA_TYPE_OFFSET);
	crc = readUint16(this->length - 2);
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
    , src(0x0000)
    , dst(0x0000)
    , dataType(0x0000)
    , crc(0x0000) {
	if (length < FRAME_MIN_LEN) {    // ensure minimum buffer size
		length = FRAME_MIN_LEN;
		buffer.resize(FRAME_MIN_LEN, 0x00);
	}
	buffer.front() = FRAME_BEGIN >> 8;
	buffer.at(FRAME_TYPE_OFFSET) = type;
	buffer.at(FRAME_DATA_LEN_OFFSET) = dataLength;
}

bool EstiaFrame::setByte(uint8_t offset, uint8_t value, bool updateCrc) {
	if (offset >= length) { return false; }

	buffer.at(offset) = value;
	if (updateCrc) { this->updateCrc(); }
	return true;
}

const uint8_t* EstiaFrame::data() const {
	return buffer.data();
}

uint8_t EstiaFrame::size() const {
	return buffer.size();
}

void EstiaFrame::updateCrc() {
	crc = crc16(buffer.data(), length - 2);
	writeUint16(length - 2, crc);
}

bool EstiaFrame::insertData(uint8_t* data, bool incHeader, bool updateCrc) {
	uint8_t len = incHeader ? dataLength : dataLength - FRAME_DATA_HEADER_LEN;
	for (uint8_t idx = 0; idx < len; idx++) {
		this->buffer.at(idx + FRAME_DATA_OFFSET) = data[idx];
	}
	if (updateCrc) { this->updateCrc(); }
	return true;
}

String EstiaFrame::stringify() {
	return stringify(this->buffer);
}

template <typename Buffer>
String EstiaFrame::stringify(const Buffer& buffer) {
	String stringifyBuffer = "";
	for (auto& byte : buffer) {
		if (byte < 0x10) { stringifyBuffer += "0"; }
		stringifyBuffer += String(byte, HEX) + " ";
	}
	stringifyBuffer.trim();
	return stringifyBuffer;
}

template String EstiaFrame::stringify<FrameBuffer>(const FrameBuffer& buffer);
template String EstiaFrame::stringify<ReadBuffer>(const ReadBuffer& buffer);

void EstiaFrame::setSrc(uint16_t src, bool updateCrc) {
	this->src = src;
	writeUint16(FRAME_SRC_OFFSET, src);
	if (updateCrc) { this->updateCrc(); }
}

void EstiaFrame::setDst(uint16_t dst, bool updateCrc) {
	this->dst = dst;
	writeUint16(FRAME_DST_OFFSET, dst);
	if (updateCrc) { this->updateCrc(); }
}

void EstiaFrame::setDataType(uint16_t dataType, bool updateCrc) {
	this->dataType = dataType;
	writeUint16(FRAME_DATA_TYPE_OFFSET, dataType);
	if (updateCrc) { this->updateCrc(); }
}

bool EstiaFrame::writeUint16(uint8_t offset, uint16_t data) {
	return writeUint16(this->buffer, offset, data);
}

uint16_t EstiaFrame::readUint16(uint8_t offset) {
	return readUint16(this->buffer, offset);
}

uint8_t EstiaFrame::checkFrame(uint8_t type, uint16_t dataType) {
	if (crc != crc16(buffer.data(), length - 2)) { return err_crc; }
	if (this->type != type) { return err_frame_type; }
	if (dataLength != length - FRAME_HEAD_AND_CRC_LEN) { return err_data_len; }
	if (this->dataType != dataType) { return err_data_type; }
	return err_ok;
}

template <typename Buffer>
bool EstiaFrame::writeUint16(Buffer& buffer, uint8_t offset, uint16_t data) {
	if (offset >= buffer.size() - 1) { return false; }

	buffer.at(offset) = static_cast<uint8_t>((data & 0xff00) >> 8);
	buffer.at(offset + 1) = static_cast<uint8_t>(data & 0x00ff);
	return true;
}

template bool EstiaFrame::writeUint16<FrameBuffer>(FrameBuffer& buffer, uint8_t offset, uint16_t data);
template bool EstiaFrame::writeUint16<ReadBuffer>(ReadBuffer& buffer, uint8_t offset, uint16_t data);

template <typename Buffer>
uint16_t EstiaFrame::readUint16(const Buffer& buffer, uint8_t offset) {
	if (offset >= buffer.size()) { return 0x0000; }
	if (offset == buffer.size() - 1) { return buffer.at(buffer.size() - 1); }

	return (buffer.at(offset) << 8) | buffer.at(offset + 1);
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

template <typename Buffer>
bool EstiaFrame::isStatusFrame(const Buffer& buffer) {
	return buffer.size() == FRAME_STATUS_LEN
	       && buffer.at(FRAME_TYPE_OFFSET) == FRAME_TYPE_STATUS
	       && buffer.at(FRAME_DATA_LEN_OFFSET) == FRAME_STATUS_DATA_LEN
	       && readUint16(buffer, FRAME_DATA_TYPE_OFFSET) == FRAME_DATA_TYPE_STATUS;
}

template bool EstiaFrame::isStatusFrame<ReadBuffer>(const ReadBuffer& buffer);
template bool EstiaFrame::isStatusFrame<FrameBuffer>(const FrameBuffer& buffer);

template <typename Buffer>
bool EstiaFrame::isStatusUpdateFrame(const Buffer& buffer) {
	return buffer.size() == FRAME_UPDATE_LEN
	       && buffer.at(FRAME_TYPE_OFFSET) == FRAME_TYPE_UPDATE
	       && buffer.at(FRAME_DATA_LEN_OFFSET) == FRAME_UPDATE_DATA_LEN
	       && readUint16(buffer, FRAME_DATA_TYPE_OFFSET) == FRAME_DATA_TYPE_STATUS;
}

template bool EstiaFrame::isStatusUpdateFrame<ReadBuffer>(const ReadBuffer& buffer);
template bool EstiaFrame::isStatusUpdateFrame<FrameBuffer>(const FrameBuffer& buffer);

template <typename Buffer>
bool EstiaFrame::isAckFrame(const Buffer& buffer) {
	return buffer.size() == FRAME_ACK_LEN
	       && buffer.at(FRAME_TYPE_OFFSET) == FRAME_TYPE_ACK
	       && buffer.at(FRAME_DATA_LEN_OFFSET) == FRAME_ACK_DATA_LEN
	       && readUint16(buffer, FRAME_DATA_TYPE_OFFSET) == FRAME_DATA_TYPE_ACK;
}

template bool EstiaFrame::isAckFrame<ReadBuffer>(const ReadBuffer& buffer);
template bool EstiaFrame::isAckFrame<FrameBuffer>(const FrameBuffer& buffer);

template <typename Buffer>
bool EstiaFrame::isDataResFrame(const Buffer& buffer) {
	return buffer.size() == FRAME_RES_DATA_LEN
	       && buffer.at(FRAME_TYPE_OFFSET) == FRAME_TYPE_RES_DATA
	       && buffer.at(FRAME_DATA_LEN_OFFSET) == FRAME_RES_DATA_DATA_LEN
	       && readUint16(buffer, FRAME_DATA_TYPE_OFFSET) == FRAME_DATA_TYPE_DATA_RESPONSE;
}

template bool EstiaFrame::isDataResFrame<ReadBuffer>(const ReadBuffer& buffer);
template bool EstiaFrame::isDataResFrame<FrameBuffer>(const FrameBuffer& buffer);
