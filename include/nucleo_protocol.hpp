#ifndef NUCLEO_PROTOCOL_
#define NUCLEO_PROTOCOL_

#include <chrono>
#include <cstdint>
#include <thread>
#include <chrono>
#include <sstream>
#include <optional>
#include <unordered_map>
#include <algorithm> 

#include "serial.hpp"


#define END_SEQ 0xEE
#define ESCAPE_CHAR 0x7E


#define NUM_SEQ 6
#define COMM_SEQ 0xAA
#define HB_SEQ 0xBB
#define SENS_0_SEQ 0xC0
#define SENS_1_SEQ 0xC1
#define SENS_2_SEQ 0xC2
#define INIT_SEQ 0xFF

static const uint8_t start_bytes[NUM_SEQ] = { INIT_SEQ, COMM_SEQ, HB_SEQ, SENS_0_SEQ, SENS_1_SEQ, SENS_2_SEQ };

enum COMM_TYPE {
    MOTOR,
    ARM
};


enum COMM_STATUS {
    OK,
    SERIAL_NOT_IN_BUFFER,
    SERIAL_INCOMPLETE_COMMUNICATION,
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

typedef std::pair<COMM_STATUS, std::optional<std::vector<uint8_t>>> packet_t;

typedef std::vector<uint8_t> keys_t;

class Protocol {
    public:
        // Constructor
        Protocol(uint8_t version, uint8_t sub_version, uint8_t address, int baudrate , bool verbose = false);
        ~Protocol();
        // Methods
        bool is_connected();

        bool connect();

        void disconnect();

        
        COMM_STATUS init(uint8_t frequency);

        ssize_t send_packet(uint8_t command, uint16_t* packet_array, size_t packet_array_length);

        packet_t get_packet(uint8_t start_byte, uint8_t end_byte = END_SEQ);

        keys_t update_buffer();


    private:

        Serial m_serial;
        uint8_t m_address;
        uint8_t m_version;
        uint8_t m_sub_version;
        std::vector<uint8_t> m_raw_buffer;
        std::unordered_map<uint8_t, packet_t> m_buffer;

};



#endif // NUCLEO_PROCOL_