// This main.cpp is for demonstration purposes only
#include <iostream>
#include "nucleo_protocol.hpp"

#define VERSION 0x01
#define SUB_VERSION 0x01
#define BAUDRATE 9600


void print_vec_(std::vector<uint8_t> val) {
    for (int i = 0; i < val.size(); i++) std::cout << std::hex << static_cast<int>(val[i]) << " ";
    std::cout << std::dec << std::endl;
}

void handle_disconnection(Protocol p) {
    if (!p.is_connected()) {
        p.connect();
        while (p.init(0x00, 100, 100) != COMM_STATUS::OK) {
            std::cout << "INIT FAILED" << std::endl;
            p.connect();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        };
        std::cout << "INIT SUCCESS" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    }
}


int main() {
    // Init variables
    Protocol p(VERSION, SUB_VERSION, 0x01, BAUDRATE, true);
    uint16_t p_motor[8] = { 0xEEEE, 0x2230, 0xffff, 0xaabb, 0xdead, 0xbeef, 0xaabb, 0x7E7E };
    uint16_t p_arm[1] = { 0xEEEE };
    uint8_t sensor_id = 0;
    uint8_t sensor_i2c_addr = 0;
    uint8_t sensor_int = 0;
    uint8_t sensor_type = SENSOR_TYPE::TEMPERATURE;

    // INIT 
    while (p.init(0x00, 0xFF, 0xFF) != COMM_STATUS::OK) {
        p.connect();
        std::cout << "INIT FAILED" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    std::cout << "INIT SUCCESS" << std::endl;


    // SET POLLING SESSION
    p.set_sensor(sensor_id, sensor_i2c_addr, sensor_int, sensor_type);

    // COMMUNICATION
    keys_t keys;
    COMM_STATUS status;


    while (true) {
        handle_disconnection(p);
        p.update_buffer();


        // Send motor data 
        p.send_packet(COMM_TYPE::MOTOR, p_motor, 8);
        p.send_packet(COMM_TYPE::ARM, p_arm, 1);
        

        packet_t hb = p.get_heartbeat();
        packet_t sensor_1 = p.get_sensor(sensor_id);
        
        std::cout << "[HB]" << std::endl;
        if (hb.first == COMM_STATUS::OK) print_vec_(hb.second.value());
        else std::cout << "PROTOCOL ERROR CODE 0x" << hb.first << std::endl;

        std::cout << "[SENSOR_1]" << std::endl;
        if (sensor_1.first == COMM_STATUS::OK) print_vec_(sensor_1.second.value());
        else std::cout << "PROTOCOL ERROR CODE 0x" << sensor_1.first << std::endl;
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    return 0;    
}