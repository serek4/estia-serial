#include <estia-serial.h>

#ifdef ARDUINO_ARCH_ESP8266
#define ESTIA_SERIAL_RX D7    // 13
#define ESTIA_SERIAL_TX D8    // 15
#elif defined(ARDUINO_ARCH_ESP32)
#define ESTIA_SERIAL_RX RX1    // 26
#define ESTIA_SERIAL_TX TX1    // 27
#endif

EstiaSerial estiaSerial(ESTIA_SERIAL_RX, ESTIA_SERIAL_TX);
u_long requestDataOffInterval = 300000;    // data update interval when heat pump is doing nothing
u_long requestDataTimer = requestDataOffInterval;
bool requestData = false;

void setup() {
	Serial.begin(115200);
	Serial.println("");
	estiaSerial.begin();
	Serial.println("Setup done!");
}

void loop() {
	switch (estiaSerial.sniffer()) {
	case EstiaSerial::sniff_frame_pending:
		Serial.println(EstiaFrame::stringify(estiaSerial.getSniffedFrame()));
		if (estiaSerial.frameAck != 0) {
			Serial.printf("frame 0x%04X acked\n", estiaSerial.getAck());
		} else if (estiaSerial.newStatusData) {
			StatusData data = estiaSerial.getStatusData();
			printStatusData(data);
			// request sensors data after extended status data received (every 30s)
			if (data.extendedData) {
				if (data.pump1 ||                                                      // when pump1 is on every 30s
				    millis() - requestDataTimer >= requestDataOffInterval - 1000) {    // when pump1 is off every 5min
					requestDataTimer = millis();
					// request update for default data points (config.h -> SENSORS_DATA_TO_REQUEST)
					estiaSerial.requestSensorsData();
					// or request update for chosen data points
					// estiaSerial.requestSensorsData({"twi", "two", "wf"}, true)
				}
			}
		}
		break;
	case EstiaSerial::sniff_idle:
		// to avoid data collisions write and request data here
		if (estiaSerial.newSensorsData) {
			for (auto& sensor : estiaSerial.getSensorsData()) {
				Serial.printf("%s :", sensor.first.c_str());
				// data is error code skip multiplier
				if (sensor.second.value <= EstiaSerial::err_not_exist) {
					Serial.print(sensor.second.value);
				} else {
					Serial.print(sensor.second.value * sensor.second.multiplier);
				}
				Serial.println();
			}
		}
		break;
	}
}

void printStatusData(StatusData& data) {
	if (data.error == StatusFrame::err_ok) {
		Serial.printf("operationMode:     %s\n", data.operationMode == 0x06 ? "heating" : "cooling");
		Serial.printf("cooling:           %s\n", data.cooling ? "on" : "off");
		Serial.printf("heating:           %s\n", data.heating ? "on" : "off");
		Serial.printf("hotWater:          %s\n", data.hotWater ? "on" : "off");
		Serial.printf("autoMode:          %s\n", data.autoMode ? "on" : "off");
		Serial.printf("quietMode:         %s\n", data.quietMode ? "on" : "off");
		Serial.printf("nightMode:         %s\n", data.nightMode ? "on" : "off");
		Serial.printf("backupHeater:      %s\n", data.backupHeater ? "on" : "off");
		Serial.printf("coolingCMP:        %s\n", data.coolingCMP ? "on" : "off");
		Serial.printf("heatingCMP:        %s\n", data.heatingCMP ? "on" : "off");
		Serial.printf("hotWaterHeater:    %s\n", data.hotWaterHeater ? "on" : "off");
		Serial.printf("hotWaterCMP:       %s\n", data.hotWaterCMP ? "on" : "off");
		Serial.printf("pump1:             %s\n", data.pump1 ? "on" : "off");
		Serial.printf("hotWaterTarget:    %u\n", data.hotWaterTarget);
		Serial.printf("zone1Target:       %u\n", data.zone1Target);
		Serial.printf("zone2Target:       %u\n", data.zone2Target);
		if (data.extendedData) {
			Serial.printf("hotWaterTarget2:   %u\n", data.hotWaterTarget2);
			Serial.printf("zone1Target2:      %u\n", data.zone1Target2);
			Serial.printf("zone2Target2:      %u\n", data.zone2Target2);
		}
		Serial.printf("defrostInProgress: %s\n", data.defrostInProgress ? "true" : "false");
		Serial.printf("nightModeActive:   %s\n", data.nightModeActive ? "true" : "false");
		Serial.printf("extendedData:      %s\n", data.extendedData ? "true" : "false");
	}
	Serial.printf("error:             %u\n", data.error);
}
