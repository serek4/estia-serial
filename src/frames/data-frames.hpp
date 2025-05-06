/*
data-frames.hpp - Estia R32 heat pump data frames
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
#include <string>
#include <unordered_map>
#include <utility>

struct RequestData {
	RequestData(uint8_t code, float multiplier);
	uint8_t code;
	float multiplier;
};

/**
* @param name requested data name
* @param code data code
* @param multiplier data modifier
*/
using RequestsMap = std::unordered_map<std::string, RequestData>;

#define REQ_DATA_SRC FRAME_SRC_DST_REMOTE
#define REQ_DATA_DST FRAME_SRC_DST_MASTER
#define REQ_DATA_BASE 0x00, 0xef, 0x00, 0x2c, 0x08, 0x00, 0x00, 0x00
#define REQ_DATA_CODE_OFFSET 17

class DataReqFrame : public EstiaFrame {
  private:
	uint8_t requestCode;

  public:
	DataReqFrame(uint8_t requestCode);
};

enum RequestCode {
	// HydroUnitData
	// CODE_HW_TARGET = 0x00,            // x1    °C    hotWaterTarget
	// CODE_HEAT_ZONE1_TARGET = 0x01,    // x1    °C    heatingZone1Target
	// CODE_HEAT_ZONE2_TARGET = 0x02,    // x1    °C    heatingZone2Target
	// CODE_RC = 0x03,                   // x1    °C    remoteSensorTemperature
	CODE_TC = 0x04,                 // x1    °C    condensedTemperature
	CODE_TWI = 0x06,                // x1    °C    waterInletTemperature
	CODE_TWO = 0x07,                // x1    °C    waterOutletTemperature
	CODE_THO = 0x08,                // x1    °C    waterHeaterOutletTemperature
	CODE_TFI = 0x09,                // x1    °C    floorInletTemperature
	CODE_TTW = 0x0a,                // x1    °C    hotWaterTemperature
	CODE_MIX = 0x0b,                // x1    step  mixingValvePosition
	CODE_LPS = 0x0e,                // x1/10 kPa   lowPressure
	CODE_SW_VER = 0x0f,             // --          softVersion
	CODE_CTRL_HW_TEMP = 0x10,       // x1    °C    controlTemperatureHotWater
	CODE_CTRL_ZONE1_TEMP = 0x11,    // x1    °C    controlTemperatureZone1
	CODE_CTRL_ZONE2_TEMP = 0x12,    // x1    °C    controlTemperatureZone2
	CODE_WF = 0xc0,                 // x10   l/min water flow
	// OutdoorUnitData
	CODE_TE = 0x60,      // x1    °C  heatExchangeTemperature
	CODE_TO = 0x61,      // x1    °C  outsideTemperature
	CODE_TD = 0x62,      // x1    °C  dischargeTemperature
	CODE_TS = 0x63,      // x1    °C  suctionTemperature
	CODE_THS = 0x65,     // x1    °C  heatSinkTemperature
	CODE_CT = 0x6a,      // x1/10 A   current
	CODE_TL = 0x6d,      // x1    °C  heatExchangerCoilTemperature
	CODE_CMP = 0x70,     // x1    Hz  compressorSpeed
	CODE_FAN1 = 0x72,    // x1    RPM outdoorFan1RPM
	CODE_FAN2 = 0x73,    // x1    RPM outdoorFan2RPM
	CODE_PMV = 0x74,     // x1/10 pls outdoorPMVPosition
	CODE_HPS = 0x7a,     // x1/10 kPa Ps highPressure
	// ServiceData
	CODE_HP_ON_TIME = 0xf0,               // x1/100 h microComputerEnergizedAccumulationTime
	CODE_HW_CMP_ON_TIME = 0xf1,           // x1/100 h hotWaterCompressorONAccumulationTime
	CODE_COOL_CMP_ON_TIME = 0xf2,         // x1/100 h coolingCompressorONAccumulationTime
	CODE_HEAT_CMP_ON_TIME = 0xf3,         // x1/100 h heatingCompressorONAccumulationTime
	CODE_PUMP1_ON_TIME = 0xf4,            // x1/100 h pump1ONAccumulationTime
	CODE_HW_E_HEATER_ON_TIME = 0xf5,      // x1/100 h hotWaterEHeaterAccumulationTime
	CODE_BACKUP_HEATER_ON_TIME = 0xf6,    // x1/100 h backupEHeaterAccumulationTime
	CODE_BOOST_HEATER_ON_TIME = 0xf7,     // x1/100 h boosterEHeaterAccumulationTime
};

/**
* @param name requested data name
* @param code data code
* @param multiplier data modifier
*/
const RequestsMap requestsMap = {
    {"tc", {CODE_TC, 1}},
    {"twi", {CODE_TWI, 1}},
    {"two", {CODE_TWO, 1}},
    {"tho", {CODE_THO, 1}},
    {"tfi", {CODE_TFI, 1}},
    {"ttw", {CODE_TTW, 1}},
    {"mix", {CODE_MIX, 1}},
    {"lps", {CODE_LPS, 10}},
    {"sw_ver", {CODE_SW_VER, 1}},
    {"ctrl_hw_temp", {CODE_CTRL_HW_TEMP, 1}},
    {"ctrl_zone1_temp", {CODE_CTRL_ZONE1_TEMP, 1}},
    {"ctrl_zone2_temp", {CODE_CTRL_ZONE2_TEMP, 1}},
    {"wf", {CODE_WF, 0.1F}},
    {"te", {CODE_TE, 1}},
    {"to", {CODE_TO, 1}},
    {"td", {CODE_TD, 1}},
    {"ts", {CODE_TS, 1}},
    {"ths", {CODE_THS, 1}},
    {"ct", {CODE_CT, 10}},
    {"tl", {CODE_TL, 1}},
    {"cmp", {CODE_CMP, 1}},
    {"fan1", {CODE_FAN1, 1}},
    {"fan2", {CODE_FAN2, 1}},
    {"pmv", {CODE_PMV, 10}},
    {"hps", {CODE_HPS, 10}},
    {"hp_on_time", {CODE_HP_ON_TIME, 100}},
    {"hw_cmp_on_time", {CODE_HW_CMP_ON_TIME, 100}},
    {"cool_cmp_on_time", {CODE_COOL_CMP_ON_TIME, 100}},
    {"heat_cmp_on_time", {CODE_HEAT_CMP_ON_TIME, 100}},
    {"pump1_on_time", {CODE_PUMP1_ON_TIME, 100}},
    {"hw_e_heater_on_time", {CODE_HW_E_HEATER_ON_TIME, 100}},
    {"backup_heater_on_time", {CODE_BACKUP_HEATER_ON_TIME, 100}},
    {"boost_heater_on_time", {CODE_BOOST_HEATER_ON_TIME, 100}}};

#define RES_DATA_SRC FRAME_SRC_DST_MASTER
#define RES_DATA_DST FRAME_SRC_DST_REMOTE
#define RES_DATA_VALUE_OFFSET 15
#define RES_DATA_EMPTY_OFFSET 14
#define RES_DATA_EMPTY_FLAG 0xa2
#define RES_DATA_NOT_EMPTY_FLAG 0x2c

class DataResFrame : public EstiaFrame {
  private:
	uint8_t checkFrame();

  public:
	enum DataError {
		err_ok,
		err_data_empty,
		err_data_len,
		err_frame_type,
		err_crc,
	};

	DataResFrame(FrameBuffer&& buffer);
	DataResFrame(FrameBuffer& buffer);
	DataResFrame(ReadBuffer& buffer);

	uint8_t error;
	int16_t value;
};
