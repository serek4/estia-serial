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
    , snifferStream(emptyString)
    , newStatusData(false)
    , statusData()
    , sensorData() {
}

void EstiaSerial::begin() {
	serial->begin(ESTIA_SERIAL_BAUD, ESTIA_SERIAL_CONFIG, rxPin, txPin);
	serial->enableIntTx(false);    //disable TX
}

String EstiaSerial::sniffer() {
	static u_long readTimer = 0;
	if (millis() - readTimer > ESTIA_SERIAL_READ_DELAY) {    // throttle serial read
		readTimer = millis();
		this->read(snifferBuffer);
		if (snifferBuffer.front() == FRAME_BEGIN) {
			if (EstiaFrame::isStatusFrame(snifferBuffer) || EstiaFrame::isStatusUpdateFrame(snifferBuffer)) {
				StatusFrame statusFrame(snifferBuffer, snifferBuffer.size());
				statusData = statusFrame.decode();
				newStatusData = true;
			}
		}
		return this->snifferFrameStringify();
	}
	return emptyString;
}

StatusData& EstiaSerial::getStatusData() {
	newStatusData = false;
	return statusData;
}

String EstiaSerial::snifferFrameStringify() {
	if (snifferBuffer.size() >= FRAME_MIN_LEN) {
		while (snifferBuffer.size() > 0) {
			if (snifferStream != emptyString && snifferBuffer.size() >= 2 && snifferBuffer.front() == 0xa0 && snifferBuffer.at(1) == 0x00) {
				// next frame began in snifferBuffer
				break;
			}
			if (snifferBuffer.front() < 0x10) { snifferStream += "0"; }
			snifferStream += String(snifferBuffer.front(), HEX) + " ";
			snifferBuffer.pop_front();
		}
		snifferStream.trim();
		String newFrame = snifferStream;
		snifferStream = emptyString;
		return newFrame;
	}
	return emptyString;
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

bool EstiaSerial::requestSensorsData(DataToRequest&& sensorsToRequest) {
	if (requestDone) {
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
	if (sensorData.count(req) == 1) {
		sensorData.at(req).value = res;
	} else {
		sensorData.emplace(req, SensorData(res, requestsMap.at(req).multiplier));
	}
	if (requestCounter >= sensorsToRequest.size() - 1) {
		requestCounter = 0;
		requestDone = true;
	} else {
		requestCounter++;
	}
	return requestDone;
}

bool EstiaSerial::requestSensorsData(DataToRequest& sensorsToRequest) {
	return requestSensorsData(std::forward<DataToRequest>(sensorsToRequest));
}

/**
* @param mode `auto` `quiet` `night`
* @param onOff `1` `0`
*/
void EstiaSerial::modeSwitch(std::string mode, uint8_t onOff) {
	if (modeByName.count(mode) == 0) { return; }
	SetModeFrame modeFrame(mode, onOff);
	this->write(modeFrame, false);
}

/**
* @param operation `heating` `hot_water`
* @param onOff `1` `0`
*/
void EstiaSerial::operationSwitch(std::string operation, uint8_t onOff) {
	if (switchOperationByName.count(operation) == 0) { return; }
	SwitchFrame switchFrame(operation, onOff);
	this->write(switchFrame, false);
}

/**
* @param mode `auto` `quiet` `night` `heating` `hot_water`
* @param onOff `1` `0`
*/
void EstiaSerial::setMode(std::string mode, uint8_t onOff) {
	if (modeByName.count(mode) != 0) { modeSwitch(mode, onOff); }
	if (switchOperationByName.count(mode) == 0) { operationSwitch(mode, onOff); }
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
	this->write(temperatureFrame, false);
}

void EstiaSerial::write(const uint8_t* buffer, uint8_t len, bool disableRx) {
	digitalWrite(LED_BUILTIN, LOW);
	if (disableRx) {
		serial->enableRx(false);    // disable RX
	}
	serial->enableIntTx(true);    // enable TX
	for (uint8_t idx; idx < len; idx++) {
		serial->write(buffer[idx]);
	}
	// serial->write(buffer, len);
	serial->enableIntTx(false);    // disable TX
	if (disableRx) {
		serial->flush();           // empty serial RX buffer
		serial->enableRx(true);    // enable RX
	}
	digitalWrite(LED_BUILTIN, HIGH);
}

void EstiaSerial::write(const EstiaFrame& frame, bool disableRx) {
	write(frame.buffer.data(), frame.buffer.size(), disableRx);
}

void EstiaSerial::read(ReadBuffer& buffer, bool byteDelay) {
	digitalWrite(LED_BUILTIN, LOW);
	while (serial->available()) {    // read response
		uint8_t newByte = serial->read();
		buffer.push_back(newByte);
		if (buffer.size() > 2) {
			if (buffer.back() == 0x00 && buffer.at(buffer.size() - 1) == 0xa0) {    // new frame already began
				break;
			}
		}
		if (byteDelay) { delay(ESTIA_SERIAL_BYTE_DELAY); }
	}
	digitalWrite(LED_BUILTIN, HIGH);
}

// https://gist.github.com/aurelj/270bb8af82f65fa645c1?permalink_comment_id=2884584#gistcomment-2884584
uint16_t EstiaSerial::crc_16(uint8_t* data, size_t len) {
	uint16_t crc = 0xffff;
	uint8_t L;
	uint8_t t;
	if (!data || len <= 0) { return crc; }
	while (len--) {
		crc ^= *data++;
		L = crc ^ (crc << 4);
		t = (L << 3) | (L >> 5);
		L ^= (t & 0x07);
		t = (t & 0xf8) ^ (((t << 1) | (t >> 7)) & 0x0f) ^ (uint8_t)(crc >> 8);
		crc = (L << 8) | t;
	}
	return crc;
}
