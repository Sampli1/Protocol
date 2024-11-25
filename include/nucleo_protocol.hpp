#ifndef NUCLEO_PROTOCOL_H
#define NUCLEO_PROTOCOL_H

#include <chrono>
#include <cstdint>
#include <thread>
#include <chrono>
#include <sstream>
#include <optional>
#include <unordered_map>
#include <vector>
#include <algorithm> 

#include "protocol_utils.hpp"
#include "serial.hpp"

#define MAX_RETRY 5
#define TIME_BETWEEN 10 // ms


class Protocol {
    public:
        // Constructor & Deconstructor
        Protocol(uint8_t address, uint8_t version, uint8_t sub_version, int baudrate, bool verbose = false);
        ~Protocol();

        // Methods
        bool is_connected();

        bool connect();

        void disconnect();
        
        COMM_STATUS init(uint8_t interval, uint8_t max_retries = MAX_RETRY, uint8_t time_between_retries = TIME_BETWEEN);

        ssize_t send_packet(uint8_t command, uint16_t* packet_array, size_t packet_array_length);

        packet_t get_packet(uint8_t start_byte, uint8_t end_byte = END_SEQ);

        bool set_sensor(uint8_t ID, uint8_t i2c_address, uint8_t interval_entry, uint8_t type);

        packet_t get_sensor(uint8_t ID);

        packet_t get_heartbeat();
        
        keys_t update_buffer();


    private:

        Serial m_serial;
        uint8_t m_address;
        uint8_t m_version;
        uint8_t m_sub_version;
        std::unordered_map<uint8_t, packet_t> m_buffer;
        bool m_verbose;

        void decode_packet(std::vector<uint8_t> &packet, std::vector<uint8_t> &keys);

        bool handle_buffer_reconstruction(std::vector<uint8_t> &packet, std::vector<uint8_t> &keys);
        
        void handle_packet_stream(std::vector<std::vector<uint8_t>> &packets);
};


#endif // NUCLEO_PROCOL_H

