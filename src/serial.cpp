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

const std::string serial_prefix = "/dev/ttyACM";

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
    for (int i = 0; i < max_tries && m_fd == -1; i++) {
        serial_interface = serial_prefix + std::to_string(i);
        m_fd = serialOpen(serial_interface.c_str(), m_baudrate);
        if (m_fd != -1) { 
            m_connected = true;
            // !!!!!!!!!!!!!!!!!!!!! TO FIX THIS THING, RE-INIT DOESNT WORK
            set_device(serial_interface);
            std::cout << "[SERIAL] " << "Connected to: " << serial_interface << std::endl;
        }
    }
        
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

bool Serial::check_connection() {
    m_fd = open(get_device().c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    std::cout << m_fd << std::endl;
    return m_fd != -1;
}

void Serial::disconnect_serial() {
    serialClose(m_fd);
}

void print_vec(std::vector<uint8_t> val) {
    for (int i = 0; i < val.size(); i++) std::cout << std::hex << static_cast<int>(val[i]) << " ";
    std::cout << std::dec << std::endl;
}


// This function get ALL the packets in the buffer
std::vector<std::vector<uint8_t>> Serial::get_byte_vectors(uint8_t terminal, uint8_t escape) {
    if (!check_connection()) return {};
    
    // Allocate e init vectors (in a very conservative way)
    std::vector<std::vector<uint8_t>> msg = {{}};

    int i = 0;    
    while (serialDataAvail(m_fd) > 0) {
        uint8_t z = serialGetchar(m_fd);
        msg[i].push_back(z);
        
        if (z == terminal) {
            if (msg[i].size() == 1) {
                i++;
                msg.push_back({});
            }
            else if (msg[i][msg[i].size() - 2] != escape) {
                i++;
                msg.push_back({});
            }
        }
    }
    
    // Remove empty vectors
    msg.erase(std::remove_if(msg.begin(), msg.end(), [](const std::vector<uint8_t>& vec) { return vec.empty(); }), msg.end());

    if (m_verbose) {
        std::cout << "RECEIVED " << msg.size() << " PACKETS" << std::endl;
        for (int i = 0; i < msg.size(); i++) print_vec(msg[i]);
    }

    return msg; 
}

ssize_t Serial::send_byte_array(std::vector<uint8_t> bytes) {
    uint8_t* arr = new uint8_t[bytes.size()];
    std::memcpy(arr, bytes.data(), bytes.size() * sizeof(uint8_t));
    ssize_t written_byte = write(m_fd, arr, bytes.size());

    if (m_verbose) {
            std::cout << "SENT: ";
            print_vec(bytes);
        }
    return written_byte;
}
