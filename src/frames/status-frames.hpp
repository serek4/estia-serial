/*
status-frames.hpp - Estia R32 heat pump status frames
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

struct StatusData {
	uint8_t error;
	bool extendedData;
	bool heating;
	bool hotWater;
	bool autoMode;
	bool quietMode;
	bool nightMode;
	bool backupHeater;
	bool heatingCMP;
	bool hotWaterHeater;
	bool hotWaterCMP;
	bool pump1;
	uint8_t hotWaterTarget;
	uint8_t heatingTarget;
	uint8_t zone2Target;
	uint8_t hotWaterTarget2;
	uint8_t heatingTarget2;
	uint8_t zone2Target2;
	bool defrostInProgress;
	bool nightModeActive;
};

#define STATUS_SRC FRAME_SRC_DST_MASTER
#define STATUS_DST FRAME_SRC_DST_BROADCAST

class StatusFrame : public EstiaFrame {
  private:
	bool longFrame;

	uint8_t checkFrame();

  public:
	enum StatusError {
		err_ok,
		err_data_len,
		err_frame_type,
		err_crc,
	};
	StatusFrame(FrameBuffer&& buffer, uint8_t length);
	StatusFrame(ReadBuffer& buffer, uint8_t length);

	uint8_t error;

	StatusData decode();
};
