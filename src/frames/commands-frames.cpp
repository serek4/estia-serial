/*
commands-frames.cpp - Estia R32 heat pump commands frames
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

#include "commands-frames.hpp"

SetModeFrame::SetModeFrame(uint8_t mode, uint8_t onOff)
    : EstiaFrame::EstiaFrame(FRAME_TYPE_SET, FRAME_SET_MODE_LEN)
    , mode(mode)
    , onOff(modeOnOff(onOff)) {
	uint8_t emptyRequest[] = {SET_MODE_BASE};
	insertData(emptyRequest, false);
	updateDataType();
	setByte(SET_MODE_CODE_OFFSET, mode, false);
	setByte(SET_MODE_VALUE_OFFSET, this->onOff);
}

SetModeFrame::SetModeFrame(std::string mode, uint8_t onOff)
    : SetModeFrame::SetModeFrame(modeByName.at(mode), onOff) {
}

uint8_t SetModeFrame::modeOnOff(uint8_t onOff) {
	switch (mode) {
		case SET_AUTO_MODE_CODE:
			return onOff;
			break;
		case SET_QUIET_MODE_CODE:
			return onOff << 2;
			break;
		case SET_NIGHT_MODE_CODE:
			return onOff << 3;
			break;
	}
	return onOff;
}

SwitchFrame::SwitchFrame(uint8_t operation, uint8_t onOff)
    : EstiaFrame::EstiaFrame(FRAME_TYPE_SET, FRAME_SWITCH_LEN)
    , operation(operation)
    , onOff(operationOnOff(onOff)) {
	uint8_t emptyRequest[] = {SWITCH_BASE};
	insertData(emptyRequest, false);
	updateDataType();
	setByte(SWITCH_VALUE_OFFSET, this->onOff);
}

SwitchFrame::SwitchFrame(std::string operation, uint8_t onOff)
    : SwitchFrame::SwitchFrame(switchOperationByName.at(operation), onOff) {
}

uint8_t SwitchFrame::operationOnOff(uint8_t onOff) {
	switch (operation) {
		case SWITCH_OPERATION_HEATING:
			return operation + onOff;
			break;
		case SWITCH_OPERATION_HOT_WATER:
			return operation + (onOff << 2);
			break;
	}
	return onOff;
}

TemperatureFrame::TemperatureFrame(uint8_t zone, uint8_t heatingTemperature, uint8_t zone2Temperature, uint8_t hotWaterTemperature)
    : EstiaFrame::EstiaFrame(FRAME_TYPE_SET, FRAME_TEMPERATURE_LEN)
    , zone(zone)
    , heatingTemperature(constrainTemp(heatingTemperature))
    , zone2Temperature(constrainTemp(zone2Temperature))
    , hotWaterTemperature(constrainTemp(hotWaterTemperature)) {
	uint8_t emptyRequest[] = {TEMPERATURE_BASE};
	insertData(emptyRequest, false);
	updateDataType();
	setByte(TEMPERATURE_CODE_OFFSET, zone, false);
	switch (zone) {
		case TEMPERATURE_HEATING_CODE:
			setByte(TEMPERATURE_HEATING_VALUE_OFFSET, convertTemp(heatingTemperature), false);
			// can this two be 0x00?
			setByte(TEMPERATURE_ZONE2_VALUE_OFFSET, convertTemp(zone2Temperature), false);
			setByte(TEMPERATURE_HOT_WATER_VALUE_OFFSET, convertTemp(hotWaterTemperature), false);
			// do i need to set this too?
			setByte(TEMPERATURE_HEATING_VALUE2_OFFSET, convertTemp(heatingTemperature));
			break;

		case TEMPERATURE_HOT_WATER_CODE:
			setByte(TEMPERATURE_HOT_WATER_VALUE_OFFSET, convertTemp(hotWaterTemperature));
			break;
	}
}

uint8_t TemperatureFrame::constrainTemp(uint8_t temperature) {
	uint8_t constrained = temperature;
	switch (zone) {
		case TEMPERATURE_HEATING_CODE:
			constrained = constrain(temperature, MIN_HEATING_TEMP, MAX_HEATING_TEMP);
			break;
		case TEMPERATURE_HOT_WATER_CODE:
			constrained = constrain(temperature, MIN_HOT_WATER_TEMP, MAX_HOT_WATER_TEMP);
			break;
	}
	return constrained;
}

uint8_t TemperatureFrame::convertTemp(uint8_t temperature) {
	return (temperature + 16) * 2;
}

ForcedDefrostFrame::ForcedDefrostFrame(uint8_t onOff)
    : EstiaFrame::EstiaFrame(FRAME_TYPE_SET, FRAME_FORCE_DEFROST_LEN)
    , onOff(onOff) {
	uint8_t emptyRequest[] = {FORCE_DEFROST_BASE};
	insertData(emptyRequest, false);
	updateDataType();
	setByte(FORCE_DEFROST_CODE_OFFSET, FORCE_DEFROST_CODE, false);
	setByte(FORCE_DEFROST_VALUE_OFFSET, this->onOff);
}

AckFrame::AckFrame(ReadBuffer& buffer)
    : EstiaFrame::EstiaFrame(readBuffToFrameBuff(buffer), FRAME_ACK_LEN) {
	error = checkFrame();
	if (error == err_ok) {
		updateDataType();
		frameCode = (this->buffer.at(ACK_FRAME_CODE_OFFSET) << 8) | this->buffer.at(ACK_FRAME_CODE_OFFSET + 1);
	}
}

uint8_t AckFrame::checkFrame() {
	if (crc != crc16(buffer.data(), length - 2)) { return err_crc; }
	if (type != FRAME_TYPE_ACK) { return err_frame_type; }
	if (dataLength != length - FRAME_HEAD_AND_CRC_LEN) { return err_data_len; }
	return err_ok;
}
