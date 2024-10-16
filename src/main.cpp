// This main.cpp is for demonstration purposes only
#include <iostream>
#include "nucleo_protocol.hpp"

#define VERSION 0x01
#define SUB_VERSION 0x01

int main() {
    Protocol p(VERSION, SUB_VERSION, 0x01, 9600, true);

    uint16_t p_motor[8] = { 0xEEEE, 0x2230, 0xffff, 0x00dd, 0xdead, 0xbeef, 0x7E7E, 0x7E7E };
    uint16_t p_arm[1] = { 0xEEEE };
    
    uint8_t f =  p.init(0x00);


    if (f == COMM_STATUS::OK) std::cout << "INIT SUCCESS" << std::endl;
    else {
        std::cout << "PROTOCOL ERROR 0x" << std::hex << std::to_string(f) << std::endl;
        return -1;
    }

    p.send_packet(COMM_TYPE::MOTOR, p_motor, 8);

    p.send_packet(COMM_TYPE::ARM, p_arm, 1);

    int i = 0;

    while (true) {
        std::pair<uint8_t ,std::optional<std::vector<uint8_t>>> res = p.get_packet();

        if (res.first != COMM_STATUS::OK) {
            std::cout << "PROTOCOL ERROR 0x" << std::hex << static_cast<int>(res.first) << std::endl;
            if (res.first == COMM_STATUS::SERIAL_NOT_ESTABLISHED || res.first == COMM_STATUS::NUCLEO_TIMEOUT) {
                p.connect();
                p.init(0x00);
            }
        }
        i++;
        if (i == 10000) break;
    }

    return 0;    
}