/*
status-frames.cpp - Estia R32 heat pump status frames
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

#include "status-frames.hpp"

StatusFrame::StatusFrame(FrameBuffer&& buffer, uint8_t length)
    : EstiaFrame::EstiaFrame(buffer, length)
    , longFrame(length == FRAME_STATUS_LEN)
    , error(0) {
	error = checkFrame();
}
StatusFrame::StatusFrame(ReadBuffer& buffer, uint8_t length)
    : StatusFrame::StatusFrame(readBuffToFrameBuff(buffer), length) {
}

StatusData StatusFrame::decode() {
	StatusData data;
	data.error = error;
	if (error == err_ok) {
		data.extendedData = longFrame;
		data.heating = (buffer.at(11) & 0xc1) == 0xc1;
		data.hotWater = (buffer.at(11) & 0xc2) == 0xc2;
		data.autoMode = (buffer.at(12) & 0x04) >> 2 == 0x01;
		data.quietMode = (buffer.at(12) & 0x10) >> 4 == 0x01;
		data.nightMode = (buffer.at(12) & 0x20) >> 5 == 0x01;
		data.backupHeater = (buffer.at(13) & 0x01) >> 0 == 0x01;
		data.heatingCMP = (buffer.at(13) & 0x02) >> 1 == 0x01;
		data.hotWaterHeater = (buffer.at(13) & 0x04) >> 2 == 0x01;
		data.hotWaterCMP = (buffer.at(13) & 0x08) >> 3 == 0x01;
		data.pump1 = (buffer.at(13) & 0x10) >> 4 == 0x01;
		data.hotWaterTarget = buffer.at(14) / 0x02 - 0x10;
		data.heatingTarget = buffer.at(15) / 0x02 - 0x10;
		data.zone2Target = buffer.at(16) / 0x02 - 0x10;
		if (longFrame) {
			data.hotWaterTarget2 = buffer.at(17) / 0x02 - 0x10;
			data.heatingTarget2 = buffer.at(18) / 0x02 - 0x10;
			data.zone2Target2 = buffer.at(19) / 0x02 - 0x10;
			data.defrostInProgress = (buffer.at(21) & 0x02) == 0x02;
			data.nightModeActive = (buffer.at(21) & 0x10) == 0x10;
		} else {
			data.defrostInProgress = (buffer.at(17) & 0x02) == 0x02;
			data.nightModeActive = (buffer.at(17) & 0x10) == 0x10;
		}
	}
	return data;
}

uint8_t StatusFrame::checkFrame() {
	if (crc != crc16(buffer.data(), length - 2)) { return err_crc; }
	if (type != (longFrame ? FRAME_TYPE_STATUS : FRAME_TYPE_UPDATE)) { return err_frame_type; }
	if (dataLength != length - FRAME_HEAD_AND_CRC_LEN) { return err_data_len; }
	return err_ok;
}

// StatusUpdateFrame::StatusUpdateFrame(FrameBuffer&& buffer)
//     : EstiaFrame::EstiaFrame(buffer, FRAME_UPDATE_LEN)
//     , error(0) {
// 	error = checkFrame();
// }
// StatusUpdateFrame::StatusUpdateFrame(ReadBuffer& buffer)
//     : StatusUpdateFrame::StatusUpdateFrame(readBuffToFrameBuff(buffer)) {
// }

// StatusData StatusUpdateFrame::decode() {
// 	StatusData data;
// 	if (error == err_ok) {
// 		data.heating = (buffer.at(11) & 0xc1) == 0xc1;
// 		data.hotWater = (buffer.at(11) & 0xc2) == 0xc2;
// 		data.autoMode = (buffer.at(12) & 0x04) >> 2 == 0x01;
// 		data.quietMode = (buffer.at(12) & 0x10) >> 4 == 0x01;
// 		data.nightMode = (buffer.at(12) & 0x20) >> 5 == 0x01;
// 		data.heatingCMP = (buffer.at(13) & 0x02) >> 1 == 0x01;
// 		data.hotWaterCMP = (buffer.at(13) & 0x08) >> 3 == 0x01;
// 		data.pump1 = (buffer.at(13) & 0x10) >> 4 == 0x01;
// 		data.hotWaterTarget = buffer.at(14) / 0x02 - 0x10;
// 		data.heatingTarget = buffer.at(15) / 0x02 - 0x10;
// 		data.zone2Target = buffer.at(16) / 0x02 - 0x10;
// 		data.nightModeActive = (buffer.at(17) & 0x10) == 0x10;
// 	}
// 	return data;
// }

// uint8_t StatusUpdateFrame::checkFrame() {
// 	if (crc != crc16(buffer.data(), length - 2)) { return err_crc; }
// 	if (type != FRAME_TYPE_UPDATE) { return err_frame_type; }
// 	if (dataLength != length - FRAME_HEAD_AND_CRC_LEN) { return err_data_len; }
// 	return err_ok;
// }
