/*
estia-serial.hpp - Serial communication with estia R32 heat pump
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

#include "config.h"
#include "frames/data-frames.hpp"
#include "frames/commands-frames.hpp"
#include "frames/status-frames.hpp"
#include <SoftwareSerial.h>
#include <map>
#include <string>
#include <vector>

#define ESTIA_SERIAL_BAUD 2400              // 2400
#define ESTIA_SERIAL_CONFIG SWSERIAL_8E1    // 8E1
#ifdef ARDUINO_ARCH_ESP32
#define LED_BUILTIN 0
#endif
#define ESTIA_SERIAL_BYTE_DELAY 5     // 4.2 ms minimum for baud 2400
#define ESTIA_SERIAL_READ_DELAY 55    // minimum valid frame is 13 Bytes so min 54.6ms between frames

#define REQUEST_RETRIES 1
#define REQUEST_TIMEOUT 250
#define REQUEST_RETRY_DELAY 15

struct SensorData {
	SensorData(int16_t value, const float multiplier);
	int16_t value;
	float multiplier;
};
using DataToRequest = std::vector<std::string>;
using EstiaData = std::map<std::string, SensorData>;

class EstiaSerial {
  private:
	int8_t rxPin;
	int8_t txPin;
	bool requestDone;
	uint8_t requestCounter;
	ReadBuffer snifferBuffer;
	String snifferStream;
	StatusData statusData;

	SoftwareSerial* serial;
	void modeSwitch(std::string mode, uint8_t onOff);
	void operationSwitch(std::string operation, uint8_t onOff);
	String snifferFrameStringify();
	void write(const uint8_t* buffer, uint8_t len, bool disableRx = true);
	void write(const EstiaFrame& frame, bool disableRx = true);
	void read(ReadBuffer& buffer, bool byteDelay = true);
	uint16_t crc_16(uint8_t* data, size_t len);    // CRC-16/MCRF4XX

  public:
	enum ResponseError {
		err_crc = -205,
		err_frame_type,
		err_data_len,
		err_data_empty,
		err_timeout,
		err_not_exist,
	};

	EstiaSerial(uint8_t rxPin, uint8_t txPin);

	bool newStatusData;
	EstiaData sensorData;

	void begin();
	String sniffer();
	StatusData& getStatusData();
	int16_t requestData(uint8_t requestCode);
	int16_t requestData(std::string request);
	bool requestSensorsData(DataToRequest&& sensorsToRequest = {SENSORS_DATA_TO_REQUEST});
	bool requestSensorsData(DataToRequest& sensorsToRequest);
	void setMode(std::string mode, uint8_t onOff);
	void setTemperature(std::string zone, uint8_t temperature);
};
