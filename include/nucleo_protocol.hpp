#ifndef NUCLEO_PROTOCOL_
#define NUCLEO_PROTOCOL_

#include <chrono>
#include <cstdint>
#include <thread>
#include <chrono>
#include <sstream>
#include <optional>
#include <unordered_map>
#include <vector>
#include <algorithm> 

#include "serial.hpp"

#define END_SEQ 0xEE
#define ESCAPE_CHAR 0x7E

#define NUM_SEQ 4
#define COMM_SEQ 0xAA
#define HB_SEQ 0xBB
#define SENS_SEQ 0xCC
#define INIT_SEQ 0xFF
#define RESERVED_BUFFER_ENTRY 0xDE


#define MAX_RETRY 5
#define TIME_BETWEEN 50 // ms

static const uint8_t start_bytes[NUM_SEQ] = { INIT_SEQ, COMM_SEQ, HB_SEQ, SENS_SEQ };
static const uint8_t bytes_to_escape[NUM_SEQ + 2] { INIT_SEQ, COMM_SEQ, HB_SEQ, SENS_SEQ, END_SEQ, ESCAPE_CHAR };


enum COMM_TYPE {
    MOTOR,
    ARM,
    SENSOR
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
        
        COMM_STATUS init(uint8_t interval, uint8_t max_retries = MAX_RETRY, uint8_t time_between_retries = TIME_BETWEEN);

        ssize_t send_packet(uint8_t command, uint16_t* packet_array, size_t packet_array_length);

        packet_t get_packet(uint8_t start_byte, uint8_t end_byte = END_SEQ);

        keys_t update_buffer(bool raw = false);


    private:

        Serial m_serial;
        uint8_t m_address;
        uint8_t m_version;
        uint8_t m_sub_version;
        std::vector<uint8_t> m_raw_buffer;
        std::unordered_map<uint8_t, packet_t> m_buffer;

};


#endif // NUCLEO_PROCOL_