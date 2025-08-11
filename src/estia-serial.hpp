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
#include "frames/commands-frames.hpp"
#include "frames/data-frames.hpp"
#include "frames/frame-fixer.hpp"
#include "frames/status-frames.hpp"
#include <SoftwareSerial.h>
#include <deque>
#include <map>
#include <string>

#define ESTIA_SERIAL_BAUD 2400              // 2400
#define ESTIA_SERIAL_CONFIG SWSERIAL_8E1    // 8E1
#ifdef ARDUINO_ARCH_ESP32
#define LED_BUILTIN 0
#endif
#define ESTIA_SERIAL_BYTE_DELAY 5        // 4.2 ms minimum for baud 2400
#define ESTIA_SERIAL_READ_TIMEOUT 190    // maximum valid frame is 45 Bytes so max 189ms transmit time
#define ESTIA_SERIAL_MIN_AVAILABLE 2     // minimum available bytes in serial buffer to start read

#define SNIFFED_FRAMES_LIMIT 64

#define REQUEST_TIMEOUT 135    // response + heartbeat transmit time
#define REQUEST_DELAY 110      // 2x shortest valid frame transmit time
#define REQUEST_RETRIES 3

#define CMD_TIMEOUT 1000
#define CMD_QUEUE_SIZE 10
#define CMD_RETRIES 2

struct SensorData {
	SensorData(int16_t value, const float multiplier);
	int16_t value;
	float multiplier;
};
using DataToRequest = std::deque<std::string>;
using EstiaData = std::map<std::string, SensorData>;
using SniffedFrames = std::deque<FrameBuffer>;
using CommandsQueue = std::deque<EstiaFrame>;

class EstiaSerial {
  private:
	int8_t rxPin;
	int8_t txPin;
	EstiaData sensorsData;
	bool requestSent;
	DataToRequest requestQueue;
	uint32_t requestTimer;
	uint8_t requestRetry;
	ReadBuffer snifferBuffer;
	FrameBuffer sniffedFrame;
	SniffedFrames sniffedFrames;
	StatusData statusData;
	bool cmdSent;
	CommandsQueue cmdQueue;
	uint32_t cmdTimer;
	uint8_t cmdRetry;

	SoftwareSerial* serial;
	FrameFixer frameFixer;
	void modeSwitch(std::string mode, uint8_t onOff);
	void operationSwitch(std::string operation, uint8_t onOff);
	bool splitSnifferBuffer(bool ignoreMinLen = false);
	bool decodeStatus(FrameBuffer& buffer);
	bool decodeAck(FrameBuffer& buffer);
	bool decodeResponse(FrameBuffer& buffer);
	void saveSensorData(uint16_t data);
	void queueCommand(EstiaFrame& command);
	bool sendCommand();
	bool sendRequest();
	void write(const uint8_t* buffer, uint8_t len, bool disableRx = true);
	bool read(ReadBuffer& buffer, bool byteDelay = true);

  public:
	enum ResponseError {
		err_data_empty = -206,
		err_data_type,
		err_data_len,
		err_frame_type,
		err_crc,
		err_timeout,
		err_not_exist,
	};
	enum SnifferState {
		sniff_idle,
		sniff_busy,
		sniff_frame_pending,
	};

	EstiaSerial(uint8_t rxPin, uint8_t txPin);

	uint16_t frameAck;
	bool newStatusData;
	bool newSensorsData;

	void begin();
	SnifferState sniffer();
	FrameBuffer getSniffedFrame();
	uint16_t getAck();
	StatusData& getStatusData();
	EstiaData& getSensorsData();
	int16_t requestData(uint8_t requestCode);
	int16_t requestData(std::string request);
	void clearSensorsData();
	bool requestSensorsData(DataToRequest&& sensorsToRequest = {SENSORS_DATA_TO_REQUEST}, bool clear = false);
	bool requestSensorsData(DataToRequest& sensorsToRequest, bool clear = false);
	void setOperationMode(std::string mode);
	void setMode(std::string mode, uint8_t onOff);
	void setTemperature(std::string zone, uint8_t temperature);
	void forceDefrost(uint8_t onOff);
	template <typename Frame>
	void write(const Frame& frame, bool disableRx = true);

	friend class SmartTarget;
};
