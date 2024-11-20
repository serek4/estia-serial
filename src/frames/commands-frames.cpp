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

// HeatingTemperatureFrame::HeatingTemperatureFrame(uint8_t temperature)
//     : EstiaFrame::EstiaFrame(FRAME_TYPE_SET, FRAME_TEMPERATURE_LEN)
//     , temperature(temperature) {
// 	uint8_t emptyRequest[] = {HEATING_TEMPERATURE_BASE};
// 	insertData(emptyRequest, false);
// 	setByte(HEATING_TEMPERATURE_VALUE_OFFSET, (temperature + 16) * 2, false);
// 	setByte(HEATING_TEMPERATURE_VALUE2_OFFSET, (temperature + 16) * 2);
// }

// HotWaterTemperatureFrame::HotWaterTemperatureFrame(uint8_t temperature)
//     : EstiaFrame::EstiaFrame(FRAME_TYPE_SET, FRAME_TEMPERATURE_LEN)
//     , temperature(temperature) {
// 	uint8_t emptyRequest[] = {HOT_WATER_TEMPERATURE_BASE};
// 	insertData(emptyRequest, false);
// 	setByte(HOT_WATER_TEMPERATURE_VALUE_OFFSET, (temperature + 16) * 2);
// }

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

TemperatureFrame::TemperatureFrame(uint8_t zone, uint8_t temperature)
    : EstiaFrame::EstiaFrame(FRAME_TYPE_SET, FRAME_TEMPERATURE_LEN)
    , zone(zone)
    , temperature(constrainTemp(temperature)) {
	uint8_t emptyRequest[] = {TEMPERATURE_BASE};
	insertData(emptyRequest, false);
	setByte(TEMPERATURE_CODE_OFFSET, zone, false);
	switch (zone) {
		case TEMPERATURE_HEATING_CODE:
			setByte(TEMPERATURE_HEATING_VALUE_OFFSET, (temperature + 16) * 2, false);
			// can this two be 0x00?
			setByte(TEMPERATURE_ZONE2_VALUE_OFFSET, 0x7a, false);
			setByte(TEMPERATURE_HOT_WATER_VALUE_OFFSET, 0x76, false);
			// do i need to set this too?
			setByte(TEMPERATURE_HEATING_VALUE2_OFFSET, (temperature + 16) * 2);
			break;

		case TEMPERATURE_HOT_WATER_CODE:
			setByte(TEMPERATURE_HOT_WATER_VALUE_OFFSET, (temperature + 16) * 2);
			break;
	}
}
