/*
estia-serial.cpp - Serial communication with estia R32 heat pump
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

#include "estia-serial.hpp"

SoftwareSerial softwareSerial;

SensorData::SensorData(int16_t value, const float multiplier)
    : value(value)
    , multiplier(multiplier) {
}

EstiaSerial::EstiaSerial(uint8_t rxPin, uint8_t txPin)
    : serial(&softwareSerial)
    , rxPin(rxPin)
    , txPin(txPin)
    , requestDone(false)
    , requestCounter(0)
    , requestRetry(0)
    , snifferBuffer()
    , sniffedFrame()
    , sniffedFrames()
    , frameAck(0)
    , newStatusData(false)
    , statusData()
    , cmdSent(false)
    , cmdQueue()
    , cmdTimer(0)
    , cmdRetry(0)
    , sensorsData() {
}

void EstiaSerial::begin() {
	serial->begin(ESTIA_SERIAL_BAUD, ESTIA_SERIAL_CONFIG, rxPin, txPin);
	serial->enableIntTx(false);    //disable TX
}

EstiaSerial::SnifferState EstiaSerial::sniffer() {
	static u_long readTimer = 0;
	if (millis() - readTimer > ESTIA_SERIAL_READ_DELAY) {    // throttle serial read
		readTimer = millis();
		this->read(snifferBuffer);
		if (!snifferBuffer.empty() && snifferBuffer.front() == FRAME_BEGIN) {
			decodeAck(snifferBuffer);
			decodeStatus(snifferBuffer);
		}
		this->splitSnifferBuffer();
	}
	if (!sniffedFrames.empty()) { return sniff_frame_pending; }
	if (!snifferBuffer.empty() || serial->available()) { return sniff_busy; }
	if (sendCommand()) { return sniff_busy; }
	return sniff_idle;
}

FrameBuffer EstiaSerial::getSniffedFrame() {
	FrameBuffer frame;
	if (!sniffedFrames.empty()) {
		frame = sniffedFrames.front();
		sniffedFrames.pop_front();
	}
	return frame;
}

void EstiaSerial::decodeStatus(ReadBuffer buffer) {
	if (buffer.size() > FRAME_STATUS_LEN) { buffer.resize(FRAME_STATUS_LEN); }
	bool isStatusFrame = EstiaFrame::isStatusFrame(buffer);

	if (!isStatusFrame && buffer.size() > FRAME_UPDATE_LEN) { buffer.resize(FRAME_UPDATE_LEN); }
	bool isUpdateFrame = EstiaFrame::isStatusUpdateFrame(buffer);

	if (isStatusFrame || isUpdateFrame) {
		StatusFrame statusFrame(buffer, buffer.size());
		if (statusFrame.error == StatusFrame::err_ok) {
			statusData = statusFrame.decode();
			newStatusData = true;
		}
	}
}

StatusData& EstiaSerial::getStatusData() {
	newStatusData = false;
	return statusData;
}

void EstiaSerial::decodeAck(ReadBuffer& buffer) {
	if (!EstiaFrame::isAckFrame(buffer)) { return; }

	AckFrame ackFrame(buffer);
	if (ackFrame.error != StatusFrame::err_ok) { return; }

	frameAck = ackFrame.frameCode;

	// command received, remove from queue
	if (ackFrame.frameCode == cmdQueue.front().dataType) {
		cmdQueue.pop_front();
		cmdRetry = 0;
		cmdSent = false;
	}
}

void EstiaSerial::queueCommand(EstiaFrame& command) {
	if (cmdQueue.size() >= CMD_QUEUE_SIZE) { return; }

	cmdQueue.push_back(command);
}

bool EstiaSerial::sendCommand() {
	// clear flag to resend command
	if (cmdSent && millis() - cmdTimer > CMD_TIMEOUT) {
		cmdRetry++;
		if (cmdRetry > CMD_RETRIES) {
			cmdQueue.pop_front();
			cmdRetry = 0;
		}
		cmdSent = false;
	}
	if (!cmdSent && !cmdQueue.empty()) {
		cmdSent = true;
		this->write(cmdQueue.front(), false);
		cmdTimer = millis();
		return true;
	}
	return false;
}

uint16_t EstiaSerial::getAck() {
	uint16_t acked = frameAck;
	frameAck = 0;
	return acked;
}

bool EstiaSerial::splitSnifferBuffer() {
	if (snifferBuffer.size() >= FRAME_MIN_LEN) {
		while (!snifferBuffer.empty()) {
			if (!sniffedFrame.empty() && snifferBuffer.size() >= 2 && snifferBuffer.front() == 0xa0 && snifferBuffer.at(1) == 0x00) {
				// next frame has already begun in snifferBuffer
				break;
			}
			sniffedFrame.push_back(snifferBuffer.front());
			snifferBuffer.pop_front();
		}
		sniffedFrames.push_back(sniffedFrame);
		sniffedFrame.clear();
		return true;
	}
	return false;
}

int16_t EstiaSerial::requestData(uint8_t requestCode) {
	DataReqFrame request(requestCode);
	this->write(request);    //send request
	uint32_t responseTimeoutTimer = millis();
	while (!serial->available()) {    // wait for response
		if (millis() - responseTimeoutTimer > REQUEST_TIMEOUT) { return err_timeout; }
		delay(ESTIA_SERIAL_BYTE_DELAY);
	}
	delay(ESTIA_SERIAL_BYTE_DELAY * 2);    // 2 bytes head start
	ReadBuffer buffer;
	this->read(buffer);               // read response into buffer
	DataResFrame response(buffer);    // create response frame from read buffer
	if (response.error != DataResFrame::err_ok) { return err_timeout + -response.error; }
	return response.value;
}

int16_t EstiaSerial::requestData(std::string request) {
	if (requestsMap.count(request) == 1) {
		return requestData((requestsMap.at(request)).code);
	}
	return err_not_exist;
}

void EstiaSerial::clearSensorsData() {
	sensorsData.clear();
}

bool EstiaSerial::requestSensorsData(DataToRequest&& sensorsToRequest, bool clear) {
	if (requestDone) {
		if (clear) { clearSensorsData(); }
		requestDone = false;
		requestCounter = 0;
	}
	std::string req = sensorsToRequest.at(requestCounter);
	int16_t res = requestData(req);
	if (res <= err_timeout && requestRetry < REQUEST_RETRIES) {
		requestRetry++;
		return requestDone;    // false
	}
	requestRetry = 0;
	if (sensorsData.count(req) == 1) {
		sensorsData.at(req).value = res;
	} else {
		sensorsData.emplace(req, SensorData(res, requestsMap.at(req).multiplier));
	}
	if (requestCounter >= sensorsToRequest.size() - 1) {
		requestCounter = 0;
		requestDone = true;
	} else {
		requestCounter++;
	}
	return requestDone;
}

bool EstiaSerial::requestSensorsData(DataToRequest& sensorsToRequest, bool clear) {
	return requestSensorsData(std::forward<DataToRequest>(sensorsToRequest), clear);
}

/**
* @param mode `auto` `quiet` `night`
* @param onOff `1` `0`
*/
void EstiaSerial::modeSwitch(std::string mode, uint8_t onOff) {
	if (modeByName.count(mode) == 0) { return; }
	SetModeFrame modeFrame(mode, onOff);
	this->queueCommand(modeFrame);
}

/**
* @param operation `heating` `hot_water`
* @param onOff `1` `0`
*/
void EstiaSerial::operationSwitch(std::string operation, uint8_t onOff) {
	if (switchOperationByName.count(operation) == 0) { return; }
	SwitchFrame switchFrame(operation, onOff);
	this->queueCommand(switchFrame);
}

/**
* @param mode `auto` `quiet` `night` `heating` `hot_water`
* @param onOff `1` `0`
*/
void EstiaSerial::setMode(std::string mode, uint8_t onOff) {
	if (modeByName.count(mode) != 0) { modeSwitch(mode, onOff); }
	if (switchOperationByName.count(mode) != 0) { operationSwitch(mode, onOff); }
}

/**
* @param zone `heating` `hot_water`
* @param temperature for heating `20-65`, for hot water `40-75`
*/
void EstiaSerial::setTemperature(std::string zone, uint8_t temperature) {
	if (temperatureByName.count(zone) == 0) { return; }
	uint8_t heating = statusData.heatingTarget;
	uint8_t zone2 = statusData.zone2Target;
	uint8_t hotWater = statusData.hotWaterTarget;
	switch (temperatureByName.at(zone)) {
		case TEMPERATURE_HEATING_CODE:
			heating = temperature;
			break;
		case TEMPERATURE_HOT_WATER_CODE:
			hotWater = temperature;
			break;
	}
	TemperatureFrame temperatureFrame(temperatureByName.at(zone), heating, zone2, hotWater);
	this->queueCommand(temperatureFrame);
}

/** Force defrost on next operation start (heating or hot water).
*
* If heating or hot water is in progress turn off and on operation
* for defrost to start now
* @param onOff `1` `0`
*/
void EstiaSerial::forceDefrost(uint8_t onOff) {
	ForcedDefrostFrame defrostFrame(onOff);
	this->queueCommand(defrostFrame);
}

void EstiaSerial::write(const uint8_t* buffer, uint8_t len, bool disableRx) {
	digitalWrite(LED_BUILTIN, LOW);
	if (disableRx) {
		serial->enableRx(false);    // disable RX
	}
	serial->enableIntTx(true);    // enable TX
	serial->write(buffer, len);
	serial->enableIntTx(false);    // disable TX
	if (disableRx) {
		serial->flush();           // empty serial RX buffer
		serial->enableRx(true);    // enable RX
	}
	digitalWrite(LED_BUILTIN, HIGH);
}

template <typename Frame>
void EstiaSerial::write(const Frame& frame, bool disableRx) {
	this->write(frame.data(), frame.size(), disableRx);
}

void EstiaSerial::read(ReadBuffer& buffer, bool byteDelay) {
	digitalWrite(LED_BUILTIN, LOW);
	while (serial->available()) {    // read response
		uint8_t newByte = serial->read();
		buffer.push_back(newByte);
		if (buffer.size() > 2) {
			if (buffer.back() == 0x00 && buffer.at(buffer.size() - 2) == 0xa0) {    // new frame already began
				break;
			}
		}
		if (byteDelay) { delay(ESTIA_SERIAL_BYTE_DELAY); }
	}
	digitalWrite(LED_BUILTIN, HIGH);
}
