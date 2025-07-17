/*
commands-frames.hpp - Estia R32 heat pump commands frames
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

#include "../config.h"
#include "frame.hpp"
#include <Arduino.h>
#include <string>
#include <unordered_map>

#define SET_MODE_SRC FRAME_SRC_DST_REMOTE
#define SET_MODE_DST FRAME_SRC_DST_MASTER
#define SET_MODE_CODE_OFFSET 11
#define SET_MODE_VALUE_OFFSET 12

#define SET_AUTO_MODE_CODE 0x01
#define SET_QUIET_MODE_CODE 0x04
#define SET_NIGHT_MODE_CODE 0x88

// auto mode on/off
// a0 00 11 0b 00 00 40 08 00 03 c4 01 00 00 00 de df -> on,  offset 12 value 0x01
// a0 00 11 0b 00 00 40 08 00 03 c4 01 01 00 00 84 03 -> off, offset 12 value 0x00
// quiet mode on/off
// a0 00 11 0b 00 00 40 08 00 03 c4 04 04 00 00 d3 e9 -> on,  offset 12 value 0x04 (1<<2)
// a0 00 11 0b 00 00 40 08 00 03 c4 04 00 00 00 b0 88 -> off, offset 12 value 0x00
// night mode on/off
// a0 00 11 0b 00 00 40 08 00 03 c4 88 08 00 00 cc 10 -> on,  offset 12 value 0x08 (1<<3)
// a0 00 11 0b 00 00 40 08 00 03 c4 88 00 00 00 0a d2 -> off, offset 12 value 0x00

using ModeByName = std::unordered_map<std::string, uint8_t>;

const ModeByName modeByName = {
    {"auto", SET_AUTO_MODE_CODE},
    {"quiet", SET_QUIET_MODE_CODE},
    {"night", SET_NIGHT_MODE_CODE}};

class SetModeFrame : public EstiaFrame {
  private:
	uint8_t mode;
	uint8_t onOff;

	uint8_t modeOnOff(uint8_t onOff);

  public:
	SetModeFrame(uint8_t mode, uint8_t onOff);
	SetModeFrame(std::string mode, uint8_t onOff);
};

// cooling/heating operation
// a0 00 11 08 00 00 40 08 00 03 c0 05 b1 7c             -> cooling, 0x05
// a0 00 11 08 00 00 40 08 00 03 c0 06 83 e7             -> heating, 0x06

#define OPERATION_MODE_SRC FRAME_SRC_DST_REMOTE
#define OPERATION_MODE_DST FRAME_SRC_DST_MASTER
#define OPERATION_MODE_OFFSET 11
#define OPERATION_MODE_COOLING 0x05
#define OPERATION_MODE_HEATING 0x06

using OperationModeByName = std::unordered_map<std::string, uint8_t>;

const OperationModeByName operationModeByName = {
    {"cooling", OPERATION_MODE_COOLING},
    {"heating", OPERATION_MODE_HEATING}};

class OperationMode : public EstiaFrame {
  private:
	uint8_t mode;

  public:
	OperationMode(uint8_t mode);
	OperationMode(std::string mode);
};


#define SWITCH_SRC FRAME_SRC_DST_REMOTE
#define SWITCH_DST FRAME_SRC_DST_MASTER
#define SWITCH_VALUE_OFFSET 11
#define SWITCH_OPERATION_COOL_HEAT 0x22
#define SWITCH_OPERATION_HOT_WATER 0x28

// cooling/heating on/off
// a0 00 11 08 00 00 40 08 00 00 41 23 8f 38 -> on,  0x23 offset 11
// a0 00 11 08 00 00 40 08 00 00 41 22 9e b1 -> off, 0x22 offset 11
// hot water on/off
// a0 00 11 08 00 00 40 08 00 00 41 2c 77 cf -> on,  0x2c offset 11
// a0 00 11 08 00 00 40 08 00 00 41 28 31 eb -> off, 0x28 offset 11

using SwitchOperationByName = std::unordered_map<std::string, uint8_t>;

const SwitchOperationByName switchOperationByName = {
    {"cooling", SWITCH_OPERATION_COOL_HEAT},
    {"heating", SWITCH_OPERATION_COOL_HEAT},
    {"hot_water", SWITCH_OPERATION_HOT_WATER}};

class SwitchFrame : public EstiaFrame {
  private:
	uint8_t operation;
	uint8_t onOff;

	uint8_t operationOnOff(uint8_t onOff);

  public:
	SwitchFrame(uint8_t operation, uint8_t onOff);
	SwitchFrame(std::string operation, uint8_t onOff);
};

#define TEMPERATURE_SRC FRAME_SRC_DST_REMOTE
#define TEMPERATURE_DST FRAME_SRC_DST_MASTER
#define TEMPERATURE_CODE_OFFSET 11
#define TEMPERATURE_COOLING_CODE 0x01
#define TEMPERATURE_HEATING_CODE 0x02
#define TEMPERATURE_HOT_WATER_CODE 0x08
#define TEMPERATURE_ZONE1_VALUE_OFFSET 12
#define TEMPERATURE_ZONE2_VALUE_OFFSET 13
#define TEMPERATURE_HOT_WATER_VALUE_OFFSET 14
#define TEMPERATURE_ZONE1_VALUE2_OFFSET 15

using TemperatureByName = std::unordered_map<std::string, uint8_t>;

const TemperatureByName temperatureByName = {
    {"cooling", TEMPERATURE_COOLING_CODE},
    {"heating", TEMPERATURE_HEATING_CODE},
    {"hot_water", TEMPERATURE_HOT_WATER_CODE}};

// cooling temperature
// a0 00 11 0c 00 00 40 08 00 03 c1 01 4a 4a 76 4a d4 3f -> cooling temperature change, offset 12, 13 and 15, value = (temp + 16) * 2
// heating temperature
// a0 00 11 0c 00 00 40 08 00 03 c1 02 5c 7a 76 5c b2 d1 -> heating temperature change, offset 12 and 15 value = (temp + 16) * 2
// hot water temperature
// a0 00 11 0c 00 00 40 08 00 03 c1 08 00 00 70 00 83 c0 -> hot water temperature change, offset 14 value = (temp + 16) * 2

class TemperatureFrame : public EstiaFrame {
  private:
	uint8_t zone;
	uint8_t zone1Temperature;
	uint8_t zone2Temperature;
	uint8_t hotWaterTemperature;

	uint8_t constrainTemp(uint8_t temperature);
	uint8_t convertTemp(uint8_t temperature);

  public:
	TemperatureFrame(uint8_t zone, uint8_t zone1Temperature, uint8_t zone2Temperature, uint8_t hotWaterTemperature);
};

#define FORCE_DEFROST_SRC FRAME_SRC_DST_REMOTE
#define FORCE_DEFROST_DST FRAME_SRC_DST_MASTER
#define FORCE_DEFROST_CODE_OFFSET 12
#define FORCE_DEFROST_VALUE_OFFSET 13

#define FORCE_DEFROST_CODE 0x46

// force defrost
// a0 00 11 0a 00 00 40 08 00 00 15 00 46 01 e7 25 -> on,  command 0x46, value 0x01
// a0 00 11 0a 00 00 40 08 00 00 15 00 46 00 f6 ac -> off, command 0x46, value 0x00

class ForcedDefrostFrame : public EstiaFrame {
  private:
	uint8_t code;
	uint8_t onOff;

  public:
	ForcedDefrostFrame(uint8_t onOff);
};

#define ACK_SRC FRAME_SRC_DST_MASTER
#define ACK_DST FRAME_SRC_DST_MASTER
#define ACK_FRAME_CODE_OFFSET 11

// ack
// a0 00 18 09 00 08 00 08 00 00 a1 00 41 c1 95 -> frame with data type 0x0041 ack'd

class AckFrame : public EstiaFrame {
  private:
  public:
	AckFrame(FrameBuffer&& buffer);
	AckFrame(FrameBuffer& buffer);
	AckFrame(ReadBuffer& buffer);

	uint16_t frameCode;
	uint8_t error;
};
