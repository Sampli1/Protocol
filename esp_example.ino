#include <Arduino.h>

#define INIT_CODE 0xFF
#define COMM_CODE 0xAA
#define HEARTBEAT_CODE 0xBB
#define END_CODE_INIT 0xEE
#define END_CODE_HB 0xEE
#define OLD_VERSION 0x10
#define THRUSTERS_FAIL 0x20

bool inited = false;
uint8_t address = 0x00;     // Indirizzo Nucleo
uint8_t version = 0x01;     // Versione
uint8_t subVersion = 0x01;  // Sub-versione

uint8_t calculate_CRC_8(const uint8_t* data, size_t length) {
    uint8_t crc = 0x00;
    uint8_t polynomial = 0x07;

    for (size_t i = 0; i < length; i++) {
        crc ^= data[i];

        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ polynomial;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

void sendInitResponse(uint8_t responseCode) {
    uint8_t packet[6];
    packet[0] = INIT_CODE;
    packet[1] = address;
    packet[2] = version;
    packet[3] = subVersion;
    packet[4] = responseCode;
    uint8_t crc = calculate_CRC_8(packet, 5);
    Serial.write(packet, 5);
    Serial.write(crc);
    Serial.write(END_CODE_INIT);
}

#define SENSOR_CODE 0xAA
#define SENSOR1_CODE 0x00
#define SENSOR2_CODE 0x01
#define SENSOR3_CODE 0x02
#define END_CODE_SENSOR 0xEE
#define ESCAPE_CHAR 0x7E
#define MAX_PACKET_SIZE 20

void sendPacketWithEscape(uint8_t* packet, size_t length) {
    uint8_t escapedPacket[MAX_PACKET_SIZE];
    size_t escapedLength = 0;

    for (size_t i = 0; i < length - 1; i++) {
        if (packet[i] == 0xEE || packet[i] == 0x7E) {
            escapedPacket[escapedLength++] = ESCAPE_CHAR;
        }
        escapedPacket[escapedLength++] = packet[i];
        if (escapedLength >= MAX_PACKET_SIZE) {
            return;
        }
    }

    escapedPacket[escapedLength++] = packet[length - 1];
    Serial.write(escapedPacket, escapedLength);
}

void sendSensorData(uint8_t sensorCode, uint16_t sensorValue) {
    uint8_t packet[9];
    packet[0] = SENSOR_CODE;
    packet[1] = 0x00;
    packet[2] = 0x00;
    packet[3] = sensorCode;
    packet[4] = 0x00;
    packet[5] = (sensorValue >> 8) & 0xFF;
    packet[6] = sensorValue & 0xFF;
    uint8_t crc = calculate_CRC_8(packet, 7);
    packet[7] = crc;

    sendPacketWithEscape(packet, 8);
    Serial.write(END_CODE_SENSOR);
}

void handleInitPacket() {
    if (Serial.available() >= 6) {
        uint8_t initCode = Serial.read();
        if (initCode == INIT_CODE) {
            uint8_t addr = Serial.read();
            uint8_t ver = Serial.read();
            uint8_t subVer = Serial.read();
            uint8_t frequency = Serial.read();
            uint8_t CRC = Serial.read();

            if (ver < version || (ver == version && subVer < subVersion)) {
                sendInitResponse(OLD_VERSION);
            } else {
                sendInitResponse(0x00);
                inited = true;
            }
        }
    }
}

void sendHeartbeat() {
    uint8_t status = random(0, 2);
    uint8_t statusCode = random(0, 8);

    uint8_t packet[5];
    packet[0] = HEARTBEAT_CODE;
    packet[1] = address;
    uint8_t combinedStatus = (status << 7) | (statusCode & 0x07);
    packet[2] = combinedStatus;
    packet[3] = END_CODE_HB;
    uint8_t crc = calculate_CRC_8(packet, 4);
    packet[4] = crc;

    sendPacketWithEscape(packet, 5);
    Serial.write(END_CODE_HB);
}

// Task per il pacchetto di heartbeat
void heartbeatTask(void* parameter) {
    while (1) {
        if (inited) {
            sendHeartbeat();
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);  // Esegui ogni 1000ms
    }
}

// Task per inviare i dati dei sensori
void sensorTask(void* parameter) {
    static unsigned long lastSensor1 = 0;
    static unsigned long lastSensor2 = 0;
    static unsigned long lastSensor3 = 0;

    while (1) {
        if (millis() - lastSensor1 >= 1 && inited) {
            lastSensor1 = millis();
            uint16_t sensor1Value = random(0, 1024);
            sendSensorData(SENSOR1_CODE, sensor1Value);
        }

        if (millis() - lastSensor2 >= 2 && inited) {
            lastSensor2 = millis();
            uint16_t sensor2Value = random(0, 1024);
            sendSensorData(SENSOR2_CODE, sensor2Value);
        }

        if (millis() - lastSensor3 >= 3 && inited) {
            lastSensor3 = millis();
            uint16_t sensor3Value = random(0, 1024);
            sendSensorData(SENSOR3_CODE, sensor3Value);
        }

        vTaskDelay(10 / portTICK_PERIOD_MS);  // Riprova ogni 100ms
    }
}

void setup() {
    Serial.begin(115200);
    // Crea i task FreeRTOS
    xTaskCreate(heartbeatTask, "Heartbeat Task", 2048, NULL, 1, NULL);
    xTaskCreate(sensorTask, "Sensor Task", 2048, NULL, 1, NULL);
}

void loop() {
    handleInitPacket();
}
