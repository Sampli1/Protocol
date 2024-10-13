// This main.cpp is for demonstration purposes only
#include <iostream>
#include "nucleo_protocol.hpp"

#define VERSION 0x00
#define SUB_VERSION 0x00

int main() {
    Protocol p(VERSION, SUB_VERSION, 0x01, true);
    uint8_t f =  p.init(0x00);
    uint16_t packet_array[8] = { 0x0001, 0x0030, 0xffff, 0x00dd, 0xdead, 0xbeef, 0x1111, 0x2222 };
    

    if (f == 0x00) std::cout << "INIT SUCCESS" << std::endl;

    p.send_packet(1, packet_array, 8);

    while (true) {
        std::vector<uint8_t> res = p.get_packet();

        if (res.size() <= 1) {
            std::cout << "PROTOCOL ERROR 0x" << std::hex << static_cast<int>(res[0]) << std::endl;
            if (res[0] == COMM_STATUS::SERIAL_NOT_ESTABLISHED || res[0] == COMM_STATUS::NUCLEO_TIMEOUT) {
                p.connect();
                p.init(0x00);
            }
        }

    }

    return 0;    
}