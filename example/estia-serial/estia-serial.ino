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
u_long requestNextTimer = 0;
u_long requestDataDelay = ESTIA_SERIAL_READ_DELAY;
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
						requestNextTimer = millis();
						requestData = true;
					}
				}
			}
			break;
		case EstiaSerial::sniff_idle:
			// to avoid data collisions write and request data here
			if (requestData && millis() - requestNextTimer >= requestDataDelay) {
				if (estiaSerial.requestSensorsData()) {    // request update for all data points
				// if (estiaSerial.requestSensorsData({"twi", "two", "wf"}), true) {    // request update for chosen data points
					requestData = false;
					for (auto& element : estiaSerial.getSensorsData()) {
						Serial.printf("%s :", element.first);
						// data is error code skip multiplier
						if (element.second.value <= EstiaSerial::err_not_exist) {
							Serial.print(element.second.value);
						} else {
							Serial.print(element.second.value * element.second.multiplier);
						}
						Serial.println();
					}
				}
				requestNextTimer = millis();
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
