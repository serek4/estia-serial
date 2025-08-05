/*
frame-fixer.hpp - Estia R32 heat pump received frames correction
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

#include "frame.hpp"
#include <vector>

struct KnownFrame {
	KnownFrame(uint8_t frameType, uint8_t dataLen, uint16_t src, uint16_t dst, uint16_t dataType);

	uint8_t frameType;
	uint8_t dataLen;
	uint16_t src;
	uint16_t dst;
	uint16_t dataType;
};

using KnownFrames = std::vector<KnownFrame>;

class FrameFixer {
  private:
	bool fixDataLength();
	bool fixStaticBytes();
	bool fixFrameType(const KnownFrame& frame);
	bool fixDataHeader(const KnownFrame& frame);

	FrameBuffer fixedBuffer;
	uint16_t crc;

  public:
	FrameFixer();

	bool fixFrame(FrameBuffer& buffer);
};
