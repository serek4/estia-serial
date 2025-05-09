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
	uint8_t blankRequest[dataLength - FRAME_DATA_HEADER_LEN] = {REQ_DATA_BASE};

	setSrc(REQ_DATA_SRC);
	setDst(REQ_DATA_DST);
	setDataType(FRAME_DATA_TYPE_DATA_REQUEST);
	insertData(blankRequest, false);
	setByte(REQ_DATA_CODE_OFFSET, requestCode, true);
}

DataResFrame::DataResFrame(FrameBuffer&& buffer)
    : EstiaFrame::EstiaFrame(buffer, FRAME_RES_DATA_LEN)
    , error(0)
    , value(0) {
	error = checkFrame();
	if (error == err_ok) {
		value = readUint16(RES_DATA_VALUE_OFFSET);
	}
}

DataResFrame::DataResFrame(FrameBuffer& buffer)
    : DataResFrame::DataResFrame(std::forward<FrameBuffer>(buffer)) {
}

DataResFrame::DataResFrame(ReadBuffer& buffer)
    : DataResFrame::DataResFrame(readBuffToFrameBuff(buffer)) {
}

uint8_t DataResFrame::checkFrame() {
	uint8_t error = EstiaFrame::checkFrame(FRAME_TYPE_RES_DATA, FRAME_DATA_TYPE_DATA_RESPONSE);
	if (error != err_ok) { return error; }

	if (readUint16(RES_DATA_EMPTY_OFFSET) == RES_DATA_FLAG_EMPTY) { return err_data_empty; }

	return err_ok;
}
