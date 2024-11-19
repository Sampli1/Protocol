#include <Arduino.h>
// Definizione dei codici
#define INIT_CODE 0xFF
#define COMM_CODE 0xAA
#define HEARTBEAT_CODE 0xBB
#define END_CODE_INIT 0xEE
#define END_CODE_HB 0xEE
// Risposte di errore
#define OLD_VERSION 0x10
#define THRUSTERS_FAIL 0x20

bool inited = false;
// Indirizzo e versioni
uint8_t address = 0x01;     // Indirizzo Nucleo
uint8_t version = 0x01;     // Versione
uint8_t subVersion = 0x01;  // Sub-versione

uint8_t calculate_CRC_8(const uint8_t* data, size_t length) {
    uint8_t crc = 0x00;  // CRC iniziale
    uint8_t polynomial = 0x07;  // Polinomio CRC-8 standard

    for (size_t i = 0; i < length; i++) {
        crc ^= data[i];  // XOR con il byte corrente

        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ polynomial;  // Shift a sinistra e XOR con il polinomio
            } else {
                crc <<= 1;  // Shift a sinistra
            }
        }
    }

    return crc;
}

void sendInitResponse(uint8_t responseCode) {
    uint8_t packet[6];  // Array per costruire il pacchetto con CRC
    packet[0] = INIT_CODE;
    packet[1] = address;
    packet[2] = version;
    packet[3] = subVersion;
    packet[4] = responseCode;

    // Calcola il CRC sui primi 5 byte del pacchetto
    uint8_t crc = calculate_CRC_8(packet, 5);

    // Invia il pacchetto con il CRC e il codice di fine
    Serial.write(packet, 5);   // Invia i 5 byte del pacchetto
    Serial.write(crc);         // Invia il CRC calcolato
    Serial.write(END_CODE_INIT);    // Invia il codice di fine pacchetto
}

#define SENSOR_CODE 0xAA
#define SENSOR1_CODE 0x00  // Codice per il sensore 1
#define SENSOR2_CODE 0x01  // Codice per il sensore 2
#define SENSOR3_CODE 0x02  // Codice per il sensore 3
#define END_CODE_SENSOR 0xEE  // Codice finale per i pacchetti sensore


// Definizione del carattere di escape
#define ESCAPE_CHAR 0x7E

// Massima dimensione del pacchetto
#define MAX_PACKET_SIZE 10  // Adatta questa dimensione in base alle tue necessità

void sendPacketWithEscape(uint8_t* packet, size_t length) {
    uint8_t escapedPacket[MAX_PACKET_SIZE];  // Pacchetto con spazio per byte aggiuntivi di escape
    size_t escapedLength = 0;  // Dimensione effettiva del pacchetto con escape

    // Itera su tutti i byte del pacchetto tranne l'ultimo (il codice di fine)
    for (size_t i = 0; i < length - 1; i++) {
        if (packet[i] == 0xEE) {
            // Se trovi 0xEE, inserisci prima l'ESCAPE_CHAR (0x7E)
            escapedPacket[escapedLength++] = ESCAPE_CHAR;
        }

        // Aggiungi il byte corrente al pacchetto "escaped"
        escapedPacket[escapedLength++] = packet[i];

        // Verifica di non superare la dimensione massima
        if (escapedLength >= MAX_PACKET_SIZE) {
            // Gestisci l'errore (puoi gestire l'overflow come preferisci)
            return;
        }
    }

    // Aggiungi l'ultimo byte del pacchetto (il codice di fine) senza modifiche
    escapedPacket[escapedLength++] = packet[length - 1];                                           

    // Invia il pacchetto "escaped"
    Serial.write(escapedPacket, escapedLength);
}



// Funzione per inviare i dati di un sensore
void sendSensorData(uint8_t sensorCode, uint16_t sensorValue) {
    uint8_t packet[9];  // Array per costruire il pacchetto
    packet[0] = SENSOR_CODE;
    packet[1] = 0x00;
    packet[2] = 0x00;
    packet[3] = sensorCode;
    packet[4] = 0x00;                
    packet[5] = (sensorValue >> 8) & 0xFF; // Byte alto del valore del sensore
    packet[6] = sensorValue & 0xFF;        // Byte basso del valore del sensore

    // Calcola il CRC sui primi 7 byte del pacchetto
    uint8_t crc = calculate_CRC_8(packet, 7);

    // Aggiungi il CRC e il codice di fine
    packet[7] = crc;


    sendPacketWithEscape(packet, 8);

    // Invia il pacchetto tramite la seriale
    Serial.write(END_CODE_SENSOR);
}


// Funzione per gestire l'inizializzazione
void handleInitPacket() {
    if (Serial.available() >= 6) { // Assicurati che ci siano almeno 6 byte disponibili
        uint8_t initCode = Serial.read();
        if (initCode == INIT_CODE) {
            uint8_t addr = Serial.read();
            uint8_t ver = Serial.read();
            uint8_t subVer = Serial.read();
            uint8_t frequency = Serial.read();  // Frequenza non utilizzata in questa simulazione
           uint8_t CRC = Serial.read();

            // Logica per controllare la versione
            if (ver < version || (ver == version && subVer < subVersion)) {
                sendInitResponse(OLD_VERSION);
            } else {
                sendInitResponse(0x00);  // Risposta positiva
                inited = true;
            }
        }
    }
}



// Funzione per inviare l'heartbeat con controllo sull'escape
void sendHeartbeat() {
    // Randomizza i valori di status e statusCode
    uint8_t status = random(0, 2);       // Status può essere 0 o 1 (OK o not OK)
    uint8_t statusCode = random(0, 8);   // Status code può variare tra 0 e 7 (3 bit)

    // Crea il pacchetto di heartbeat
    uint8_t packet[5];  // Array per costruire il pacchetto di heartbeat
    packet[0] = HEARTBEAT_CODE;
    packet[1] = address;

    // Combina status e statusCode in un unico byte
    uint8_t combinedStatus = (status << 7) | (statusCode & 0x07); // Status in bit 7 e statusCode nei bit 0-2
    packet[2] = combinedStatus; // Status e StatusCode

    // Inserisci 0xEE (END_CODE_HB) all'interno del pacchetto, per simulare il caso in cui
    // questo valore sia parte del messaggio, non solo come codice di fine.
    packet[3] = END_CODE_HB;

    // Calcola il CRC sui primi 4 byte del pacchetto
    uint8_t crc = calculate_CRC_8(packet, 4);

    // Aggiungi il CRC alla fine del pacchetto
    packet[4] = crc;

    // Invia il pacchetto dopo aver controllato gli escape
    sendPacketWithEscape(packet, 5);

    // Invia il codice di fine pacchetto
    Serial.write(END_CODE_HB);
}


void setup() {
    Serial.begin(9600);  // Inizializza la comunicazione seriale
    delay(1000);         // Attendi un attimo per l'inizializzazione
}

void loop() {
    // Gestisci il pacchetto di inizializzazione
    handleInitPacket();

    // Simula il pacchetto di heartbeat ogni 1 secondo
    static unsigned long lastHeartbeat = 0;
    if (millis() - lastHeartbeat >= 2 && inited) {
        lastHeartbeat = millis();
        sendHeartbeat(); // Invia pacchetto di heartbeat con valori predefiniti
    }

    // Simula i pacchetti dei sensori
    static unsigned long lastSensor1 = 0;
    static unsigned long lastSensor2 = 0;
    static unsigned long lastSensor3 = 0;

    if (millis() - lastSensor1 >= 20 && inited) {
        lastSensor1 = millis();
        uint16_t sensor1Value = random(0, 1024);  // Valore casuale per il sensore 1
        sendSensorData(SENSOR1_CODE, sensor1Value);
    }

    if (millis() - lastSensor2 >= 10 && inited) {
        lastSensor2 = millis();
        uint16_t sensor2Value = random(0, 1024);  // Valore casuale per il sensore 2
        sendSensorData(SENSOR2_CODE, sensor2Value);
    }

    if (millis() - lastSensor3 >= 5 && inited) {
        lastSensor3 = millis();
        uint16_t sensor3Value = random(0, 1024);  // Valore casuale per il sensore 3
        sendSensorData(SENSOR3_CODE, sensor3Value);
    }
}
