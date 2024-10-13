#include <iostream>
#include <chrono>
#include <thread>
#include <fcntl.h>
#include "serial.hpp"
#include "wiringSerial.h"

const std::string serial_prefix = "/dev/ttyACM";

Serial::Serial() {
    m_connected = false;
    m_device = "";
    m_fd = -1;
    m_verbose = false;
}

int Serial::connect_serial() {
    std::string serial_interface;
    int max_tries = 10;

    std::cout << "[SERIAL] Trying connecting to serial..." << std::endl;
 
    // Try to connect to different serial interfaces
    for (int i = 0; i < max_tries && m_fd == -1; i++) {
        serial_interface = serial_prefix + std::to_string(i);
        m_fd = serialOpen(serial_interface.c_str(), 9600);
        if (m_fd != -1) { 
            std::cout << "[SERIAL] " << "Connected to: " << serial_interface << std::endl;
            m_connected = true;
            m_device = serial_interface;
        }
    }

    return m_connected;
}

int Serial::get_available_data() {
    return check_connection() ? serialDataAvail(m_fd) : -1;   
}

void Serial::set_verbose(bool val) {
    m_verbose = val;
}

bool Serial::check_connection() {
    m_fd = open(m_device.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    return m_fd != -1;
}

void print_vec(std::vector<uint8_t> val) {
    for (int i = 0; i < val.size(); i++) std::cout << std::hex << static_cast<int>(val[i]) << " ";
    std::cout << std::endl;
}



std::vector<uint8_t> Serial::get_byte_vector(unsigned char terminal) {
    if (!check_connection()) return {};
    std::vector<uint8_t> msg; 
    uint cooldown = 100; // Allow to fill the buffer
    while (serialDataAvail(m_fd) > 0) {
        char z = serialGetchar(m_fd);
        msg.push_back(static_cast<uint8_t>(z)); 
        if (z == terminal) break; 
        std::this_thread::sleep_for(std::chrono::milliseconds(cooldown));
    }

    if (m_verbose) {
        std::cout << "RECEIVED: ";
        print_vec(msg);
    }

    return msg; 
}

ssize_t Serial::send_byte_array(std::vector<uint8_t> bytes) {

    uint8_t* arr = new uint8_t[bytes.size()];
    std::memcpy(arr, bytes.data(), bytes.size() * sizeof(uint8_t));
    ssize_t written_byte = write(m_fd, arr, bytes.size());

    if (written_byte > 0 && m_verbose) {
        std::cout << "SEND: ";
        print_vec(bytes);
    }
    else {
        std::cout << "SEND ERROR" << std::endl; 
    }

    return written_byte;
}
