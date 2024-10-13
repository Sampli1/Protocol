#ifndef NUCLEO_PROTOCOL_
#define NUCLEO_PROTOCOL_

#include <chrono>
#include <cstdint>
#include <thread>
#include <chrono>
#include <sstream>
#include <algorithm> 

#include "serial.hpp"

#define INIT_SEQ 0xFF
#define COMM_SEQ 0xAA
#define SIZE_COMM_MOT 8 
#define SIZE_COMM_ARM 1
#define HB_SEQ 0xBB
#define SIZE_HB 3
#define TIMEOUT 3000 // ms
#define END_SEQ_INIT 0xDD
#define END_SEQ_COMM 0xCC
#define END_SEQ_HB 0xEE



enum COMM_STATUS {
    OK,
    SERIAL_NOT_ESTABLISHED,
    PI_OLD_VERSION,
    NUCLEO_OLD_VERSION,
    NUCLEO_TIMEOUT,
    NUCLEO_INVALID_ANSWER,
    CRC_FAILED
};

enum HB_OK_STATUS {
    READY,
    PROCESSING_PWM,
};

enum HB_ERROR_STATUS {
    NOT_READY,
    THRUSTER_FAIL
};

class Protocol {
    public:
        // Constructor
        Protocol(uint8_t version, uint8_t sub_version, uint8_t adress, bool verbose = false);

        // Methods
        bool connect();

        uint8_t init(uint8_t frequency);

        void send_packet(uint8_t command, uint16_t* packet_array, size_t packet_array_length);

        std::vector<uint8_t> get_packet(uint timeout = TIMEOUT, uint8_t start_byte = HB_SEQ, uint8_t end_byte = END_SEQ_HB);


    private:
        Serial serial;
        uint8_t address;
        uint8_t version;
        uint8_t sub_version;
};



#endif // NUCLEO_PROCOL_