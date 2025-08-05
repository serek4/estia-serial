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
    , newSensorsData(false)
    , requestSent(false)
    , requestQueue()
    , requestTimer(0)
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
    , frameFixer()
    , sensorsData() {
}

void EstiaSerial::begin() {
	serial->begin(ESTIA_SERIAL_BAUD, ESTIA_SERIAL_CONFIG, rxPin, txPin);
	serial->enableIntTx(false);    //disable TX
}

EstiaSerial::SnifferState EstiaSerial::sniffer() {
	static u_long readTimer = 0;
	bool timeout = !snifferBuffer.empty() && millis() - readTimer >= ESTIA_SERIAL_READ_TIMEOUT;
	if (serial->available() >= ESTIA_SERIAL_MIN_AVAILABLE || timeout) {
		bool newFrame = this->read(snifferBuffer);
		readTimer = millis();
		if (this->splitSnifferBuffer(newFrame || timeout)) {
			for (auto& frame : sniffedFrames) {
				frameFixer.fixFrame(frame);
				if (EstiaFrame::readUint16(frame, 0) != FRAME_BEGIN) { continue; }
				if (decodeStatus(frame)) { continue; }
				if (decodeAck(frame)) { continue; }
				decodeResponse(frame);
			}
		}
	}
	if (!sniffedFrames.empty()) { return sniff_frame_pending; }
	if (!snifferBuffer.empty() || serial->available()) { return sniff_busy; }
	if (sendCommand()) { return sniff_busy; }
	if (sendRequest()) { return sniff_busy; }
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

bool EstiaSerial::decodeStatus(FrameBuffer& buffer) {
	if (!(EstiaFrame::isStatusFrame(buffer) || EstiaFrame::isStatusUpdateFrame(buffer))) { return false; }

	StatusFrame statusFrame(buffer, buffer.size());
	if (statusFrame.error == StatusFrame::err_ok) {
		statusData = statusFrame.decode();
		newStatusData = true;
	}
	return true;
}

StatusData& EstiaSerial::getStatusData() {
	newStatusData = false;
	return statusData;
}

EstiaData& EstiaSerial::getSensorsData() {
	newSensorsData = false;
	return sensorsData;
}

bool EstiaSerial::decodeAck(FrameBuffer& buffer) {
	if (!EstiaFrame::isAckFrame(buffer)) { return false; }

	AckFrame ackFrame(buffer);
	if (ackFrame.error != StatusFrame::err_ok) { return true; }

	frameAck = ackFrame.frameCode;

	// command received, remove from queue
	if (cmdSent && ackFrame.frameCode == cmdQueue.front().dataType) {
		cmdQueue.pop_front();
		cmdRetry = 0;
		cmdSent = false;
	}
	return true;
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

bool EstiaSerial::sendRequest() {
	if (requestQueue.empty()) { return false; }

	// discard invalid requests
	while (requestsMap.count(requestQueue.front()) == 0) {
		requestQueue.pop_front();
		if (requestQueue.empty()) { break; }
	}
	// request timeout
	if (requestSent && !requestQueue.empty() && millis() - requestTimer >= REQUEST_TIMEOUT) {
		requestRetry++;
		if (requestRetry > REQUEST_RETRIES) {
			saveSensorData(err_timeout);
			requestQueue.pop_front();
			requestRetry = 0;
		}
		requestSent = false;
	}
	// last queue element was popped
	if (requestQueue.empty()) {
		newSensorsData = true;
	}
	if (!requestSent && !requestQueue.empty() && !cmdSent && millis() - requestTimer >= REQUEST_DELAY) {
		this->write(DataReqFrame(requestsMap.at(requestQueue.front()).code));
		requestTimer = millis();
		requestSent = true;
		return true;
	}
	return false;
}

bool EstiaSerial::decodeResponse(FrameBuffer& buffer) {
	if (!EstiaFrame::isDataResFrame(buffer)) { return false; }
	if (requestQueue.empty()) { return true; }

	requestTimer = millis();
	DataResFrame resFrame(buffer);
	if (resFrame.error != DataResFrame::err_ok) {
		resFrame.value = err_timeout + -resFrame.error;
		requestRetry++;
		if (requestRetry <= REQUEST_RETRIES) {
			requestSent = false;
			return true;
		}
	}

	saveSensorData(resFrame.value);

	// remove request from queue
	requestQueue.pop_front();
	requestRetry = 0;
	requestSent = false;

	if (requestQueue.empty()) {
		newSensorsData = true;
	}
	return true;
}

void EstiaSerial::saveSensorData(uint16_t data) {
	if (sensorsData.count(requestQueue.front()) == 1) {
		sensorsData.at(requestQueue.front()).value = data;
	} else {
		sensorsData.emplace(requestQueue.front(), SensorData(data, requestsMap.at(requestQueue.front()).multiplier));
	}
}

bool EstiaSerial::splitSnifferBuffer(bool ignoreMinLen) {
	if (!ignoreMinLen && snifferBuffer.size() < FRAME_MIN_LEN) { return false; }

	uint8_t frameSize = 0;
	while (!snifferBuffer.empty()) {
		// calculate expected frame length from data
		if (frameSize == 0 && sniffedFrame.size() >= FRAME_HEAD_LEN
		    && EstiaFrame::readUint16(sniffedFrame, 0) == FRAME_BEGIN) {
			frameSize = sniffedFrame.at(FRAME_DATA_LEN_OFFSET) + FRAME_HEAD_AND_CRC_LEN;
		}
		// next frame begin detected in sniffer buffer
		if (!sniffedFrame.empty() && EstiaFrame::readUint16(snifferBuffer, 0) == FRAME_BEGIN) {
			// frame shorter than expected and shorter than max
			if (sniffedFrame.size() < frameSize && frameSize <= FRAME_MAX_LEN
			    && sniffedFrame.size() <= FRAME_MAX_LEN) {
				// read remaining data
				this->read(snifferBuffer);
				// push 0xa0 byte to current frame
				sniffedFrame.push_back(snifferBuffer.front());
				snifferBuffer.pop_front();
				// and continue
				continue;
			}
			// next frame has already begun in sniffer buffer
			break;
		}
		// check for joined two frames (first probably malformed)
		if (frameSize != 0 && sniffedFrame.size() > frameSize) {
			// check sniffed frame for next frame begin byte (skip first byte)
			for (uint8_t idx = 1; idx < sniffedFrame.size(); idx++) {
				if (EstiaFrame::readUint16(sniffedFrame, idx) == FRAME_BEGIN) {
					FrameBuffer firstFrame(sniffedFrame.begin(), sniffedFrame.begin() + idx);
					sniffedFrame.erase(sniffedFrame.begin(), sniffedFrame.begin() + idx);
					sniffedFrames.push_back(firstFrame);
					frameSize = 0;
					break;
				}
			}
		}
		sniffedFrame.push_back(snifferBuffer.front());
		snifferBuffer.pop_front();
	}
	sniffedFrames.push_back(sniffedFrame);
	while (sniffedFrames.size() >= SNIFFED_FRAMES_LIMIT) {
		sniffedFrames.pop_front();
	}
	sniffedFrame.clear();

	return true;
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
	splitSnifferBuffer();                  // read out data in buffer
	snifferBuffer.clear();
	this->read(snifferBuffer);
	DataResFrame response(snifferBuffer);
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
	if (!requestQueue.empty()) { return false; }    // request in progress

	newSensorsData = false;
	if (clear) { clearSensorsData(); }
	for (auto& sensor : sensorsToRequest) {
		if (requestsMap.count(sensor) == 0) {
			continue;
		}
		requestQueue.push_back(sensor);
	}
	return true;
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
* @param mode `cooling` `heating`
*/
void EstiaSerial::setOperationMode(std::string mode) {
	if (operationModeByName.count(mode) == 0) { return; }

	OperationMode operationMode(mode);
	this->queueCommand(operationMode);
}

/**
* @param operation `cooling` `heating` `hot_water`
* @param onOff `1` `0`
*/
void EstiaSerial::operationSwitch(std::string operation, uint8_t onOff) {
	if (switchOperationByName.count(operation) == 0) { return; }

	// set operation mode (for cooling and heating)
	if (operationModeByName.count(operation) != 0 && statusData.operationMode != operationModeByName.at(operation)) {
		setOperationMode(operation);
	}
	SwitchFrame switchFrame(operation, onOff);
	this->queueCommand(switchFrame);
}

/**
* @param mode `auto` `quiet` `night` `cooling` `heating` `hot_water`
* @param onOff `1` `0`
*/
void EstiaSerial::setMode(std::string mode, uint8_t onOff) {
	if (modeByName.count(mode) != 0) { modeSwitch(mode, onOff); }
	if (switchOperationByName.count(mode) != 0) { operationSwitch(mode, onOff); }
}

/**
* @param zone `cooling` `heating` `hot_water`
* @param temperature for cooling `7-25`, for heating `20-65`, for hot water `40-75`
*/
void EstiaSerial::setTemperature(std::string zone, uint8_t temperature) {
	if (temperatureByName.count(zone) == 0) { return; }
	uint8_t zone1 = statusData.zone1Target;
	uint8_t zone2 = statusData.zone2Target;
	uint8_t hotWater = statusData.hotWaterTarget;
	switch (temperatureByName.at(zone)) {
	case TEMPERATURE_COOLING_CODE:
		zone1 = temperature;
		zone2 = temperature;
		break;

	case TEMPERATURE_HEATING_CODE:
		zone1 = temperature;
		break;

	case TEMPERATURE_HOT_WATER_CODE:
		hotWater = temperature;
		break;
	}
	TemperatureFrame temperatureFrame(temperatureByName.at(zone), zone1, zone2, hotWater);
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

bool EstiaSerial::read(ReadBuffer& buffer, bool byteDelay) {
	if (!serial->available()) { return false; }

	digitalWrite(LED_BUILTIN, LOW);
	while (serial->available()) {
		buffer.push_back(serial->read());
		if (byteDelay) { delay(ESTIA_SERIAL_BYTE_DELAY); }
		if (buffer.size() > 2 && EstiaFrame::readUint16(buffer, buffer.size() - 2) == FRAME_BEGIN) {    // new frame already began
			break;
		}
	}
	digitalWrite(LED_BUILTIN, HIGH);
	return static_cast<bool>(serial->available());
}
