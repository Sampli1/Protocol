#include <iostream>
#include <chrono>
#include <thread>
#include <fcntl.h>
#include "serial.hpp"
#include <termios.h>
#include <unistd.h>
#include <vector>
#include <algorithm>
#include "wiringSerial.h"

const std::vector<std::string> serial_prefixes = {"/dev/ttyUSB", "/dev/ttyACM"};

Serial::Serial() {
    m_connected = false;
    m_fd = -1;
    m_verbose = false;
    m_baudrate = 9600;
}

int Serial::connect_serial() {
    std::string serial_interface;
    int max_tries = 10;

    std::cout << "[SERIAL] Trying connecting to serial..." << std::endl;
 
    // Try to connect to different serial interfaces
    for (std::string serial_prefix: serial_prefixes) {
        for (int i = 0; i < max_tries; i++) {
            serial_interface = serial_prefix + std::to_string(i);
            m_fd = serialOpen(serial_interface.c_str(), m_baudrate);
            if (m_fd != -1) { 
                m_connected = true;
                set_device(serial_interface);
                std::cout << "[SERIAL] " << "Connected to: " << serial_interface << std::endl;
                break;
            }
        }
    }

    m_fd = open(m_device.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);

    tcflush(m_fd, TCIOFLUSH);
    
    
    return m_connected;
}

int Serial::get_available_data() {
    return check_connection() ? serialDataAvail(m_fd) : -1;   
}

void Serial::set_verbose(bool val) {
    m_verbose = val;
}

void Serial::set_baudrate(int baudrate) {
    m_baudrate = baudrate;
}

void Serial::set_device(std::string serial_interface) {
    m_device = serial_interface;
}

std::string Serial::get_device() {
    return m_device;    
}

#include <sys/stat.h>
bool Serial::check_connection() {
    return access(m_device.c_str(), F_OK) == 0;
}

void Serial::disconnect_serial() {
    serialClose(m_fd);
}

void print_vec(std::vector<uint8_t> val) {
    for (int i = 0; i < val.size(); i++) std::cout << "  " << std::hex << static_cast<int>(val[i]) << " ";
    std::cout << std::dec << std::endl;
}

#define MAX_PACKETS 100
#define MAX_PACKET_SIZE 256

// This function get ALL the packets in the buffer
std::vector<std::vector<uint8_t>> Serial::get_byte_vectors(uint8_t terminal, uint8_t escape) {
    if (!check_connection()) return {};
    uint8_t buffer[MAX_PACKETS][MAX_PACKET_SIZE] = {};
    size_t packet_lengths[MAX_PACKETS] = {};
    size_t packet_count = 0;

    while (serialDataAvail(m_fd) > 0 && packet_count < MAX_PACKETS) {
        uint8_t z = serialGetchar(m_fd);
        if (packet_lengths[packet_count] < MAX_PACKET_SIZE) {
            buffer[packet_count][packet_lengths[packet_count]++] = z;
        }
        
        if (z == terminal) {
            if (packet_lengths[packet_count] == 1 || 
                buffer[packet_count][packet_lengths[packet_count] - 2] != escape) {
                if (++packet_count < MAX_PACKETS) {
                    packet_lengths[packet_count] = 0;
                }
            }
        }
    }

    std::vector<std::vector<uint8_t>> msg;
    for (size_t i = 0; i <= packet_count; ++i) {
        if (packet_lengths[i] > 0) {
            msg.emplace_back(buffer[i], buffer[i] + packet_lengths[i]);
        }
    }

    if (m_verbose) {
        std::cout << "[SERIAL] RECEIVED " << msg.size() << " PACKETS" << std::endl;
        
        for (size_t i = 0; i < msg.size(); i++) print_vec(msg[i]);
    }

    return msg;
}


ssize_t Serial::send_byte_array(std::vector<uint8_t> bytes) {
    ssize_t written_byte = write(m_fd, bytes.data(), bytes.size());
    if (m_verbose) {
        std::cout << "[SERIAL] SENT: " << std::endl;
        print_vec(bytes);
    }
    return written_byte;
}
