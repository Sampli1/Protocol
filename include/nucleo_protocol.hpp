#ifndef NUCLEO_PROTOCOL_
#define NUCLEO_PROTOCOL_

#include <chrono>
#include <cstdint>
#include <thread>
#include <chrono>
#include <sstream>
#include <optional>
#include <algorithm> 

#include "serial.hpp"

#define INIT_SEQ 0xFF
#define COMM_SEQ 0xAA
#define SIZE_COMM_MOT 8 
#define SIZE_COMM_ARM 1
#define HB_SEQ 0xBB
#define SIZE_HB 3
#define TIMEOUT 3000 // ms
#define END_SEQ 0xEE
#define ESCAPE_CHAR 0x7E

enum COMM_TYPE {
    MOTOR,
    ARM
};


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
        Protocol(uint8_t version, uint8_t sub_version, uint8_t adress, int baudrate , bool verbose = false);


        // Methods
        bool is_connected();

        bool connect();

        COMM_STATUS init(uint8_t frequency);

        ssize_t send_packet(uint8_t command, uint16_t* packet_array, size_t packet_array_length);

        std::pair<COMM_STATUS ,std::optional<std::vector<uint8_t>>> get_packet(uint timeout = TIMEOUT, uint8_t start_byte = HB_SEQ, uint8_t end_byte = END_SEQ);

    private:

        Serial serial;
        uint8_t m_address;
        uint8_t m_version;
        uint8_t m_sub_version;
};



#endif // NUCLEO_PROCOL_