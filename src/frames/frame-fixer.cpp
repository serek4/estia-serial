/*
frame-fixer.cpp - Estia R32 heat pump received frames correction
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

#include "frame-fixer.hpp"


KnownFrames knownFrames = {
    KnownFrame(FRAME_TYPE_CTRL_FRAME, FRAME_HEARTBEAT_DATA_LEN, FRAME_SRC_DST_MASTER, FRAME_SRC_DST_BROADCAST, FRAME_DATA_TYPE_HEARTBEAT),      // heartbeat
    KnownFrame(FRAME_TYPE_STATUS2, FRAME_STATUS2_DATA_LEN, FRAME_SRC_DST_REMOTE, FRAME_SRC_DST_MASTER, FRAME_DATA_TYPE_STATUS),                 // remote status 30s
    KnownFrame(FRAME_TYPE_STATUS, FRAME_STATUS_DATA_LEN, FRAME_SRC_DST_MASTER, FRAME_SRC_DST_BROADCAST, FRAME_DATA_TYPE_STATUS),                // master status 30s
    KnownFrame(FRAME_TYPE_STATUS, FRAME_SHORT_STATUS_DATA_LEN, FRAME_SRC_DST_MASTER, FRAME_SRC_DST_BROADCAST, FRAME_DATA_TYPE_SHORT_STATUS),    // master status 30m
    KnownFrame(FRAME_TYPE_UPDATE, FRAME_UPDATE_DATA_LEN, FRAME_SRC_DST_MASTER, FRAME_SRC_DST_BROADCAST, FRAME_DATA_TYPE_STATUS),                // master status update
    KnownFrame(FRAME_TYPE_RES_DATA, FRAME_RES_DATA_DATA_LEN, FRAME_SRC_DST_MASTER, FRAME_SRC_DST_REMOTE, FRAME_DATA_TYPE_DATA_RESPONSE),        // data response
    KnownFrame(FRAME_TYPE_ACK, FRAME_ACK_DATA_LEN, FRAME_SRC_DST_MASTER, FRAME_SRC_DST_MASTER, FRAME_DATA_TYPE_ACK),                            // ack 1
    KnownFrame(FRAME_TYPE_ACK, FRAME_ACK_DATA_LEN, FRAME_SRC_DST_MASTER, FRAME_SRC_DST_REMOTE, FRAME_DATA_TYPE_ACK),                            // ack 2
};

KnownFrame::KnownFrame(uint8_t frameType, uint8_t dataLen, uint16_t src, uint16_t dst, uint16_t dataType)
    : frameType(frameType)
    , dataLen(dataLen)
    , src(src)
    , dst(dst)
    , dataType(dataType)
    , len(dataLen + FRAME_HEAD_AND_CRC_LEN) {}

FrameFixer::FrameFixer()
    : fixedBuffer()
    , crc() {
	fixedBuffer.reserve(FRAME_MAX_LEN);
}

bool FrameFixer::fixFrame(FrameBuffer& buffer) {
	if (buffer.size() < FRAME_MIN_LEN - 2) { return false; }

	this->crc = EstiaFrame::readUint16(buffer, buffer.size() - 2);
	if (crc == EstiaFrame::crc16(buffer.data(), buffer.size() - 2)) { return true; }

	this->fixedBuffer = buffer;

	if (this->addMissingBytes()) {
		buffer.swap(fixedBuffer);
		return true;
	}

	if (fixedBuffer.size() < FRAME_MIN_LEN) { return false; }

	if (this->fixDataLength()) {
		buffer.swap(fixedBuffer);
		return true;
	}

	if (this->fixStaticBytes()) {
		buffer.swap(fixedBuffer);
		return true;
	}

	// fix frame specific bytes
	for (auto& frame : knownFrames) {
		if (fixedBuffer.at(FRAME_DATA_LEN_OFFSET) != frame.dataLen) { continue; }

		if (this->fixFrameType(frame)) {
			buffer.swap(fixedBuffer);
			return true;
		}
		if (this->fixDataHeader(frame)) {
			buffer.swap(fixedBuffer);
			return true;
		}
	}

	return false;
};

bool FrameFixer::addMissingBytes() {
	if (EstiaFrame::readUint16(fixedBuffer, 0) == FRAME_BEGIN) { return false; }

	for (auto& frame : knownFrames) {
		if (fixedBuffer.front() == 0x00 && fixedBuffer.size() == frame.len - 1) {
			fixedBuffer.insert(fixedBuffer.begin(), 0xa0);
			if (crc == EstiaFrame::crc16(fixedBuffer.data(), fixedBuffer.size() - 2)) { return true; }
			break;
		} else if (fixedBuffer.front() == frame.frameType && fixedBuffer.size() == frame.len - 2) {
			fixedBuffer.insert(fixedBuffer.begin(), {0xa0, 0x00});
			if (crc == EstiaFrame::crc16(fixedBuffer.data(), fixedBuffer.size() - 2)) { return true; }
			break;
		}
	}
	return false;
}

bool FrameFixer::fixDataLength() {
	if (fixedBuffer.at(FRAME_DATA_LEN_OFFSET) + FRAME_HEAD_AND_CRC_LEN == fixedBuffer.size()) { return false; }

	fixedBuffer.at(FRAME_DATA_LEN_OFFSET) = fixedBuffer.size() - FRAME_HEAD_AND_CRC_LEN;
	if (crc == EstiaFrame::crc16(fixedBuffer.data(), fixedBuffer.size() - 2)) { return true; }
	return false;
}

bool FrameFixer::fixStaticBytes() {
	EstiaFrame::writeUint16(fixedBuffer, 0, FRAME_BEGIN);
	fixedBuffer.at(FRAME_DATA_HEADER_OFFSET) == 0x00;
	if (crc == EstiaFrame::crc16(fixedBuffer.data(), fixedBuffer.size() - 2)) { return true; }
	return false;
}

bool FrameFixer::fixFrameType(const KnownFrame& frame) {
	if (fixedBuffer.at(FRAME_TYPE_OFFSET) == frame.frameType) { return false; }

	fixedBuffer.at(FRAME_TYPE_OFFSET) = frame.frameType;
	if (crc == EstiaFrame::crc16(fixedBuffer.data(), fixedBuffer.size() - 2)) { return true; }
	return false;
}

bool FrameFixer::fixDataHeader(const KnownFrame& frame) {
	EstiaFrame::writeUint16(fixedBuffer, FRAME_SRC_OFFSET, frame.src);
	EstiaFrame::writeUint16(fixedBuffer, FRAME_DST_OFFSET, frame.dst);
	EstiaFrame::writeUint16(fixedBuffer, FRAME_DATA_TYPE_OFFSET, frame.dataType);
	if (crc == EstiaFrame::crc16(fixedBuffer.data(), fixedBuffer.size() - 2)) { return true; }
	return false;
}
