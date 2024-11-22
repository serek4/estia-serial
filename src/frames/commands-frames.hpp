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

#define SET_MODE_CODE_OFFSET 11
#define SET_MODE_VALUE_OFFSET 12
#define SET_MODE_BASE 0x00, 0x00, 0x40, 0x08, 0x00, 0x03, 0xc4, 0x00, 0x00, 0x00, 0x00

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

#define SWITCH_VALUE_OFFSET 11
#define SWITCH_BASE 0x00, 0x00, 0x40, 0x08, 0x00, 0x00, 0x41, 0x00
#define SWITCH_OPERATION_HEATING 0x22
#define SWITCH_OPERATION_HOT_WATER 0x28

// heating on/off
// a0 00 11 08 00 00 40 08 00 00 41 23 8f 38 -> on  0x23 offset 11
// a0 00 11 08 00 00 40 08 00 00 41 22 9e b1 -> off 0x22 offset 11
// hot water on/off
// a0 00 11 08 00 00 40 08 00 00 41 2c 77 cf -> on  0x2c offset 11
// a0 00 11 08 00 00 40 08 00 00 41 28 31 eb -> off 0x28 offset 11

using SwitchOperationByName = std::unordered_map<std::string, uint8_t>;

const SwitchOperationByName switchOperationByName = {
    {"heating", SWITCH_OPERATION_HEATING},
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

// // heating temperature
// // a0 00 11 0c 00 00 40 08 00 03 c1 02 5c 7a 76 5c b2 d1 -> heating temperature change, offset 12 and 15 value = (temp + 16) * 2

// #define HEATING_TEMPERATURE_VALUE_OFFSET 12
// #define HEATING_TEMPERATURE_VALUE2_OFFSET 15
// #define HEATING_TEMPERATURE_BASE 0x00, 0x00, 0x40, 0x08, 0x00, 0x03, 0xc1, 0x02, 0x5c, 0x7a, 0x76, 0x5c

// class HeatingTemperatureFrame : public EstiaFrame {
//   private:
// 	uint8_t temperature;

//   public:
// 	HeatingTemperatureFrame(uint8_t temperature);
// };

// // hot water temperature
// // a0 00 11 0c 00 00 40 08 00 03 c1 08 00 00 70 00 83 c0 -> hot water temperature change, offset 14 value = (temp + 16) * 2

// #define HOT_WATER_TEMPERATURE_VALUE_OFFSET 14
// #define HOT_WATER_TEMPERATURE_BASE 0x00, 0x00, 0x40, 0x08, 0x00, 0x03, 0xc1, 0x08, 0x00, 0x00, 0x70, 0x00

// class HotWaterTemperatureFrame : public EstiaFrame {
//   private:
// 	uint8_t temperature;

//   public:
// 	HotWaterTemperatureFrame(uint8_t temperature);
// };

#define TEMPERATURE_BASE 0x00, 0x00, 0x40, 0x08, 0x00, 0x03, 0xc1, 0x00, 0x00, 0x00, 0x00, 0x00
#define TEMPERATURE_CODE_OFFSET 11
#define TEMPERATURE_HEATING_CODE 0x02
#define TEMPERATURE_HOT_WATER_CODE 0x08
#define TEMPERATURE_HEATING_VALUE_OFFSET 12
#define TEMPERATURE_ZONE2_VALUE_OFFSET 13
#define TEMPERATURE_HOT_WATER_VALUE_OFFSET 14
#define TEMPERATURE_HEATING_VALUE2_OFFSET 15

using TemperatureByName = std::unordered_map<std::string, uint8_t>;

const TemperatureByName temperatureByName = {
    {"heating", TEMPERATURE_HEATING_CODE},
    {"hot_water", TEMPERATURE_HOT_WATER_CODE}};

// heating temperature
// a0 00 11 0c 00 00 40 08 00 03 c1 02 5c 7a 76 5c b2 d1 -> heating temperature change, offset 12 and 15 value = (temp + 16) * 2
// hot water temperature
// a0 00 11 0c 00 00 40 08 00 03 c1 08 00 00 70 00 83 c0 -> hot water temperature change, offset 14 value = (temp + 16) * 2

class TemperatureFrame : public EstiaFrame {
  private:
	uint8_t zone;
	uint8_t heatingTemperature;
	uint8_t zone2Temperature;
	uint8_t hotWaterTemperature;

	uint8_t constrainTemp(uint8_t temperature);
	uint8_t convertTemp(uint8_t temperature);

  public:
	TemperatureFrame(uint8_t zone, uint8_t heatingTemperature, uint8_t zone2Temperature, uint8_t hotWaterTemperature);
};
