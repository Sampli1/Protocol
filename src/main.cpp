// This main.cpp is for demonstration purposes only
#include <iostream>
#include "nucleo_protocol.hpp"

#define VERSION 0x01
#define SUB_VERSION 0x01
#define BAUDRATE 9600


void print_vec_(std::vector<uint8_t> val) {
    std::cout << "dimension: " << val.size() << std::endl;
    for (int i = 0; i < val.size(); i++) std::cout << std::hex << static_cast<int>(val[i]) << " ";
    std::cout << std::dec << std::endl;
}

int main() {
    Protocol p(VERSION, SUB_VERSION, 0x01, BAUDRATE, true);
    uint16_t p_motor[8] = { 0xEEEE, 0x2230, 0xffff, 0xaabb, 0xdead, 0xbeef, 0xaabb, 0x7E7E };
    uint16_t p_arm[1] = { 0xEEEE };

    // INIT 
    while (p.init(0x00) != COMM_STATUS::OK) {
        p.connect();
        std::cout << "INIT FAILED" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    std::cout << "INIT SUCCESS" << std::endl;



    // SET POLLING SESSION

    // TODO

    // COMMUNICATION
    keys_t keys;
    COMM_STATUS status;
    p.send_packet(COMM_TYPE::MOTOR, p_motor, 8);
    p.send_packet(COMM_TYPE::ARM, p_arm, 1);


    while (true) {
        if (p.is_connected()) keys  = p.update_buffer();
        else {
            p.connect();
            status = p.init(0x00);
            while (status != COMM_STATUS::OK) {
                std::cout << "INIT FAILED" << std::endl;
                p.connect();
                status = p.init(0x00);
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            };
            std::cout << "INIT SUCCESS" << std::endl;
            keys  = p.update_buffer();
        }



        if (keys.size() == 0) std::cout << "Empty" << std::endl;
        else {
            std::cout << "KEYS" << std::endl;
            print_vec_(keys);
            for (uint8_t key : keys) {

                packet_t packet = p.get_packet(key);

                if (packet.first == COMM_STATUS::OK) {
                    std::cout << "Received:" << std::endl;
                    print_vec_(packet.second.value());
                }

            }
        }


        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    return 0;    
}