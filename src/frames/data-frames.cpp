/*
data-frames.cpp - Estia R32 heat pump data frames
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

#include "data-frames.hpp"

RequestData::RequestData(uint8_t code, float multiplier)
    : code(code)
    , multiplier(multiplier) {
}

DataReqFrame::DataReqFrame(uint8_t requestCode)
    : EstiaFrame::EstiaFrame(FRAME_TYPE_REQ_DATA, FRAME_REQ_DATA_LEN)
    , requestCode(requestCode) {
	uint8_t blankRequest[dataLength] = {REQ_DATA_BASE};
	insertData(blankRequest, false);
	setByte(REQ_DATA_CODE_OFFSET, requestCode);
}

DataResFrame::DataResFrame(FrameBuffer&& buffer)
    : EstiaFrame::EstiaFrame(buffer, FRAME_RES_DATA_LEN)
    , error(0)
    , value(0) {
	error = checkFrame();
	if (error == err_ok) {
		value = (this->buffer.at(RES_DATA_VALUE_OFFSET) << 8) | this->buffer.at(RES_DATA_VALUE_OFFSET + 1);
	}
}

DataResFrame::DataResFrame(FrameBuffer& buffer)
    : DataResFrame::DataResFrame(std::forward<FrameBuffer>(buffer)) {
}

DataResFrame::DataResFrame(ReadBuffer& buffer)
    : DataResFrame::DataResFrame(readBuffToFrameBuff(buffer)) {
}

uint8_t DataResFrame::checkFrame() {
	if (crc != crc16(buffer.data(), length - 2)) { return err_crc; }
	if (type != FRAME_TYPE_RES_DATA) { return err_frame_type; }
	if (dataLength != length - FRAME_HEAD_AND_CRC_LEN) { return err_data_len; }
	if (buffer.at(RES_DATA_EMPTY_OFFSET) == RES_DATA_EMPTY_FLAG) { return err_data_empty; }
	return err_ok;
}
